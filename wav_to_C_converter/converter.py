import os
import re
import numpy as np
from scipy.io import wavfile
from scipy import signal

# Explicitly set the path to your MIT KEMAR dataset folder
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
    target_azs = [i * 5 for i in range(37)] # 0 to 180 degrees
    
    left_matrix = np.zeros((14, 37, 23), dtype=int)
    right_matrix = np.zeros((14, 37, 23), dtype=int)

    def angle_dist(x, y):
        return min(abs(x - y), 360 - abs(x - y))

    print("Reconstructing Right Ear with Polyphase Anti-Aliasing Resampler...")
    
    for e_idx, target_e in enumerate(target_elevs):
        if target_e not in catalog:
            continue
            
        available_azs = list(catalog[target_e].keys())
        
        for a_idx, target_a in enumerate(target_azs):
            # --- LEFT EAR: Direct Lookup ---
            closest_az_left = min(available_azs, key=lambda x: angle_dist(x, target_a))
            fs_l, data_left = wavfile.read(catalog[target_e][closest_az_left])
            
            # --- RIGHT EAR: Mirrored Lookup (360 - target) ---
            target_a_right = (360 - target_a) % 360
            closest_az_right = min(available_azs, key=lambda x: angle_dist(x, target_a_right))
            fs_r, data_right = wavfile.read(catalog[target_e][closest_az_right])
            
            # Safely extract Mono channels
            frames_l = min(128, len(data_left))
            frames_r = min(128, len(data_right))
            
            left_channel = data_left[:frames_l, 0] if len(data_left.shape) == 2 else data_left[:frames_l]
            right_channel = data_right[:frames_r, 0] if len(data_right.shape) == 2 else data_right[:frames_r]
            
            # THE FIX: Polyphase Filtering prevents FFT Phase-Cancellation!
            # 8000 Hz / 44100 Hz = 80 / 441
            left_8k = signal.resample_poly(left_channel, 80, 441)[:23]
            right_8k = signal.resample_poly(right_channel, 80, 441)[:23]
            
            left_matrix[e_idx, a_idx] = np.round(left_8k).astype(int)
            right_matrix[e_idx, a_idx] = np.round(right_8k).astype(int)

    output_filename = 'hrtf_matrix_with_elev.h'
    print(f"\nWriting True 3D Stereo data to {output_filename}...")
    
    with open(output_filename, 'w') as f:
        f.write("#ifndef HRTF_MATRIX_8K_FIXED_H\n")
        f.write("#define HRTF_MATRIX_8K_FIXED_H\n\n")
        f.write("// Mirrored, Polyphase Resampled HRTF 3D Matrices (8kHz)\n")
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