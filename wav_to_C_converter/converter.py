import numpy as np
from scipy.io import wavfile
from scipy import signal
import os

def build_hrtf_matrix(base_path, output_filename="hrtf_matrix.h", target_rate=8000):
    # Loop from 0 to 175 degrees in steps of 5
    angles = range(0, 185, 5) 
    num_angles = len(angles)
    
    left_matrix = []
    right_matrix = []
    array_length = 0
    
    for i in angles:
        # 1. Format the path correctly with :03d to ensure 3-digit zero-padding
        filename = f"H0e{i:03d}a.wav"
        full_path = os.path.join(base_path, filename)
        
        print(f"Processing {filename}...")
        
        try:
            original_rate, data = wavfile.read(full_path)
            
            left_channel = data[:, 0]
            right_channel = data[:, 1]
            
            # Downsample to 8kHz
            if original_rate != target_rate:
                num_target_samples = int(len(left_channel) * target_rate / original_rate)
                left_ready = signal.resample(left_channel, num_target_samples)
                right_ready = signal.resample(right_channel, num_target_samples)
            else:
                left_ready = left_channel
                right_ready = right_channel
                
            left_ready = np.round(left_ready).astype(np.int16)
            right_ready = np.round(right_ready).astype(np.int16)
            
            array_length = len(left_ready)
            left_matrix.append(left_ready)
            right_matrix.append(right_ready)
            
        except Exception as e:
            print(f"Failed on {filename}: {e}")
            return
            
    # 2. Write the 2D Matrix directly to a C Header
    print(f"\nGenerating {output_filename}...")
    with open(output_filename, 'w') as f:
        f.write(f"// Auto-generated {target_rate}Hz HRTF Matrix\n\n")
        f.write(f"#define NUM_ANGLES {num_angles}\n")
        f.write(f"#define HRTF_LENGTH {array_length}\n\n")
        
        # Write Left Matrix
        f.write(f"const short hrtf_left_matrix[{num_angles}][{array_length}] = {{\n")
        for arr in left_matrix:
            f.write("    {" + ", ".join(map(str, arr)) + "},\n")
        f.write("};\n\n")
        
        # Write Right Matrix
        f.write(f"const short hrtf_right_matrix[{num_angles}][{array_length}] = {{\n")
        for arr in right_matrix:
            f.write("    {" + ", ".join(map(str, arr)) + "},\n")
        f.write("};\n")
        
    print("Success! Matrix generated.")

# Run it using 'r' to specify a raw string, preventing Windows path errors
build_hrtf_matrix(r"C:\\Users\\alexa\\Downloads\\diffuse\\elev0")