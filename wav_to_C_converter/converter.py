import os
import re
import numpy as np
from scipy.io import wavfile
from scipy import signal

# Explicitly set the path to your MIT KEMAR dataset folder
DATASET_PATH = r"C:\Users\alexa\Downloads\diffuse"

def build_hrtf_matrices():
    print(f"Scanning directory: {DATASET_PATH} ...")
    
    # 1. Catalog all available WAV files by Elevation and Azimuth recursively
    catalog = {}
    for root, dirs, files in os.walk(DATASET_PATH):
        for filename in files:
            if filename.lower().endswith('.wav'):
                # Matches MIT KEMAR format: H-40e013a.wav or H00e000a.wav
                match = re.search(r'H(-?\d+)e(\d+)a', filename, re.IGNORECASE)
                if match:
                    elev = int(match.group(1))
                    az = int(match.group(2))
                    
                    if elev not in catalog:
                        catalog[elev] = {}
                    
                    # Store the full absolute path so we can open it from anywhere
                    catalog[elev][az] = os.path.join(root, filename)

    if not catalog:
        print(f"Error: No valid HRTF .wav files found in {DATASET_PATH} or its sub-folders.")
        print("Double check that the folder path is correct and contains the .wav files.")
        return

    # Define the required 14x37 grid 
    target_elevs = [-40, -30, -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
    target_azs = [i * 5 for i in range(37)] # 0, 5, 10 ... 180

    # Initialize empty 3D matrices
    left_matrix = np.zeros((14, 37, 23), dtype=int)
    right_matrix = np.zeros((14, 37, 23), dtype=int)

    print(f"Found files across {len(catalog)} elevation levels.")
    print("De-interleaving stereo data, interpolating angles, and downsampling to 8kHz...")
    
    for e_idx, target_e in enumerate(target_elevs):
        if target_e not in catalog:
            print(f"Warning: Missing elevation {target_e} in directory! Padding with zeros.")
            continue
            
        available_azs = list(catalog[target_e].keys())
        
        for a_idx, target_a in enumerate(target_azs):
            # 2. Nearest Neighbor Interpolation (Fixes the zero gaps)
            closest_az = min(available_azs, key=lambda x: abs(x - target_a))
            filepath = catalog[target_e][closest_az]
            
            # 3. Read raw WAV file
            fs, data = wavfile.read(filepath)
            
            # 4. Extract and De-interleave Stereo Channels
            frames_to_take = min(128, len(data))
            
            if len(data.shape) == 2:
                left_channel = data[:frames_to_take, 0]   # Channel 0
                right_channel = data[:frames_to_take, 1]  # Channel 1
            else:
                left_channel = data[:frames_to_take]
                right_channel = data[:frames_to_take]
            
            # 5. Downsample to exactly 23 samples
            left_8k = signal.resample(left_channel, 23)
            right_8k = signal.resample(right_channel, 23)
            
            left_matrix[e_idx, a_idx] = np.round(left_8k).astype(int)
            right_matrix[e_idx, a_idx] = np.round(right_8k).astype(int)

    # 6. Write out the clean C header file back into your GitHub folder
    output_filename = 'hrtf_matrix_8k_fixed.h'
    print(f"\nWriting clean data to {output_filename}...")
    
    with open(output_filename, 'w') as f:
        f.write("#ifndef HRTF_MATRIX_8K_FIXED_H\n")
        f.write("#define HRTF_MATRIX_8K_FIXED_H\n\n")
        f.write("// De-interleaved, Interpolated, & Downsampled HRTF 3D Matrices (8kHz)\n")
        f.write("// Dimensions: [14 Elevations] x [37 Azimuths] x [23 Samples]\n\n")
        
        # Write Left Matrix passing the target_elevs list
        f.write("const short hrtf_left_matrix[14][37][23] = {\n")
        write_matrix_to_file(f, left_matrix, target_elevs)
        f.write("};\n\n")
        
        # Write Right Matrix passing the target_elevs list
        f.write("const short hrtf_right_matrix[14][37][23] = {\n")
        write_matrix_to_file(f, right_matrix, target_elevs)
        f.write("};\n\n")
        
        f.write("#endif // HRTF_MATRIX_8K_FIXED_H\n")

    print(f"Success! Your audio data is properly formatted and saved in {os.getcwd()}.")

def write_matrix_to_file(f, matrix, target_elevs):
    for elev_idx in range(14):
        # Insert the elevation comment here
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