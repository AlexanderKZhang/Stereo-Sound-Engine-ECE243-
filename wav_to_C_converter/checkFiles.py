# Generate the list from 0 to 180 in steps of 5
file_list = []
for azimuth in range(0, 185, 5):
    # Format the number to be exactly 3 digits
    filename = f"H0e{azimuth:03d}a.wav"
    file_list.append(filename)

# Print the total count to verify (should be 37 files)
print(f"Total files generated: {len(file_list)}")

# Print the first few and last few to double-check
print(file_list[:5])
print("...")
print(file_list[-5:])