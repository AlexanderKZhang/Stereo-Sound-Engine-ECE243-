import os
import re
import numpy as np
from scipy.io import wavfile

DATASET_PATH = r"C:\Users\alexa\Downloads\diffuse"

def build_hrtf_matrices():
    print(f"Scanning directory: {DATASET_PATH} ...")
    
    catalog = {}
    for root, dirs, files in os.walk(DATASET_PATH):
        for filename in files:
            if filename.lower().endswith('.wav'):
                match = re.search(r'H(-?\d+)e(\d+)a', filename, re.IGNORECASE)
                if match:
                    elev = int(match.group(1))
                    az = int(match.group(2))
                    if elev not in catalog:
                        catalog[elev] = {}
                    catalog[elev][az] = os.path.join(root, filename)

    if not catalog:
        print("Error: No valid HRTF .wav files found.")
        return

    target_elevs = [-40, -30, -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
    target_azs = [i * 5 for i in range(37)]  # 0 to 180 degrees ONLY
    
    left_matrix_raw = np.zeros((14, 37, 23), dtype=float)
    right_matrix_raw = np.zeros((14, 37, 23), dtype=float)

    # Nearest Neighbor extraction to preserve spikes
    decimation_indices = [int(i * (44100 / 8000)) for i in range(23)]

    def angle_dist(x, y):
        return min(abs(x - y), 360 - abs(x - y))

    print("Extracting TRUE STEREO arrays...")
    
    for e_idx, target_e in enumerate(target_elevs):
        if target_e not in catalog: continue
        available_azs = list(catalog[target_e].keys())
        
        for a_idx, target_a in enumerate(target_azs):
            # Find the closest available azimuth (safely handles Elev 70/80 gaps)
            closest_az = min(available_azs, key=lambda x: angle_dist(x, target_a))
            fs, data = wavfile.read(catalog[target_e][closest_az])
            
            frames = min(128, len(data))
            
            # TRUE STEREO EXTRACTION
            if len(data.shape) == 2:
                left_channel = data[:frames, 0]   # Channel 0: Left Ear
                right_channel = data[:frames, 1]  # Channel 1: Right Ear
            else:
                left_channel = data[:frames]
                right_channel = data[:frames]
            
            left_8k = left_channel[decimation_indices]
            right_8k = right_channel[decimation_indices]
            
            left_matrix_raw[e_idx, a_idx] = left_8k
            right_matrix_raw[e_idx, a_idx] = right_8k

    # Normalize to prevent Nios II 32-bit Integer Overflow
    global_max = max(np.max(np.abs(left_matrix_raw)), np.max(np.abs(right_matrix_raw)))
    scale_factor = 4104.0 / global_max

    left_matrix = np.round(left_matrix_raw * scale_factor).astype(int)
    right_matrix = np.round(right_matrix_raw * scale_factor).astype(int)

    output_filename = 'hrtf_matrix_with_elev.h'
    print(f"\nWriting Separate 2D Matrices to {output_filename}...")
    
    elev_names = ["minus40", "minus30", "minus20", "minus10", "0", "10", "20", "30", "40", "50", "60", "70", "80", "90"]
    
    with open(output_filename, 'w') as f:
        f.write("#ifndef HRTF_MATRIX_8K_FIXED_H\n")
        f.write("#define HRTF_MATRIX_8K_FIXED_H\n\n")
        f.write("#define NUM_ANGLES 37\n")
        f.write("#define HRTF_LENGTH 23\n\n")
        
        # Write individual 2D arrays
        for e_idx, name in enumerate(elev_names):
            f.write(f"const short hrtf_left_elev_{name}[37][23] = {{\n")
            for az in range(37):
                row_str = ", ".join(map(str, left_matrix[e_idx, az]))
                f.write(f"  {{ {row_str} }}")
                if az < 36: f.write(",")
                f.write("\n")
            f.write("};\n\n")

            f.write(f"const short hrtf_right_elev_{name}[37][23] = {{\n")
            for az in range(37):
                row_str = ", ".join(map(str, right_matrix[e_idx, az]))
                f.write(f"  {{ {row_str} }}")
                if az < 36: f.write(",")
                f.write("\n")
            f.write("};\n\n")
            
        # Write fast CPU lookup tables
        f.write("// Fast CPU Lookup Tables\n")
        f.write("typedef const short (*hrtf_ptr)[23];\n\n")
        
        f.write("const hrtf_ptr left_matrices[14] = {\n")
        for name in elev_names: f.write(f"  hrtf_left_elev_{name},\n")
        f.write("};\n\n")
        
        f.write("const hrtf_ptr right_matrices[14] = {\n")
        for name in elev_names: f.write(f"  hrtf_right_elev_{name},\n")
        f.write("};\n\n")
        
        f.write("#endif // HRTF_MATRIX_8K_FIXED_H\n")

    print(f"Success! True Stereo 3D Matrices Built.")

if __name__ == "__main__":
    build_hrtf_matrices()