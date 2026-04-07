import os
import re
import numpy as np
from scipy.io import wavfile

DATASET_PATH = r"C:\\Users\\alexa\Downloads\diffuse"

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
    target_azs = [i * 5 for i in range(37)] 
    
    left_matrix_raw = np.zeros((14, 37, 23), dtype=float)
    right_matrix_raw = np.zeros((14, 37, 23), dtype=float)

    # Nearest Neighbor extraction to preserve spikes and eliminate filter delays
    decimation_indices = [int(i * (44100 / 8000)) for i in range(23)]

    print("Extracting arrays and preventing C Integer Overflow...")
    
    for e_idx, target_e in enumerate(target_elevs):
        if target_e not in catalog:
            continue
            
        available_azs = list(catalog[target_e].keys())
        
        for a_idx, target_a in enumerate(target_azs):
            closest_az = min(available_azs, key=lambda x: abs(x - target_a))
            fs, data = wavfile.read(catalog[target_e][closest_az])
            
            frames = min(128, len(data))
            
            if len(data.shape) == 2:
                left_channel = data[:frames, 0]
                right_channel = data[:frames, 1]
            else:
                left_channel = data[:frames]
                right_channel = data[:frames]
            
            left_8k = left_channel[decimation_indices]
            right_8k = right_channel[decimation_indices]
            
            left_matrix_raw[e_idx, a_idx] = left_8k
            right_matrix_raw[e_idx, a_idx] = right_8k

    # =================================================================
    # THE OVERFLOW FIX: Normalize the global maximum exactly to 4104
    # =================================================================
    global_max = max(np.max(np.abs(left_matrix_raw)), np.max(np.abs(right_matrix_raw)))
    scale_factor = 4104.0 / global_max

    left_matrix = np.round(left_matrix_raw * scale_factor).astype(int)
    right_matrix = np.round(right_matrix_raw * scale_factor).astype(int)

    output_filename = 'hrtf_matrix_with_elev.h'
    print(f"\nWriting Safe, Normalized 3D Stereo data to {output_filename}...")
    
    with open(output_filename, 'w') as f:
        f.write("#ifndef HRTF_MATRIX_8K_FIXED_H\n")
        f.write("#define HRTF_MATRIX_8K_FIXED_H\n\n")
        f.write("// Normalized HRTF 3D Matrices (8kHz) - Scaled to prevent 32-bit Integer Overflow\n")
        f.write("// Dimensions: [14 Elevations] x [37 Azimuths] x [23 Samples]\n\n")
        
        f.write("#define NUM_ANGLES 37\n")
        f.write("#define HRTF_LENGTH 23\n\n")
        
        f.write("const short hrtf_left_matrix[14][37][23] = {\n")
        write_matrix_to_file(f, left_matrix, target_elevs)
        f.write("};\n\n")
        
        f.write("const short hrtf_right_matrix[14][37][23] = {\n")
        write_matrix_to_file(f, right_matrix, target_elevs)
        f.write("};\n\n")
        
        f.write("#endif // HRTF_MATRIX_8K_FIXED_H\n")

    print(f"Success! Your audio data is properly formatted and saved.")

def write_matrix_to_file(f, matrix, target_elevs):
    for elev_idx in range(14):
        f.write(f"  // Elevation: {target_elevs[elev_idx]} degrees\n")
        f.write("  {\n")
        for az in range(37):
            row_str = ", ".join(map(str, matrix[elev_idx, az]))
            f.write(f"    {{ {row_str} }}")
            if az < 36:
                f.write(",")
            f.write("\n")
        f.write("  }")
        if elev_idx < 13:
            f.write(",")
        f.write("\n")

if __name__ == "__main__":
    build_hrtf_matrices()