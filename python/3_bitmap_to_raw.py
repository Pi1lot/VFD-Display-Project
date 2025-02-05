from PIL import Image
import os

# Directories for source and converted images
input_folder = "images_bitmap"  # Folder containing input images
output_folder = "images_raw"  # Folder for converted RAW files

# Resulting raw image size
SCREEN_WIDTH = 48
SCREEN_HEIGHT = 48

# Create the output folder if it doesn't exist
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

# Function to convert an image to RAW format
def convert_to_raw(input_path, output_path):
    try:
        # Load the image
        img = Image.open(input_path)

        # Resize to match the screen size
        img = img.resize((SCREEN_WIDTH, SCREEN_HEIGHT))

        # Ensure the image is in black and white (mode "1")
        img = img.convert("1")

        # Save the raw data in RAW format
        with open(output_path, "wb") as f:
            f.write(img.tobytes())

        print(f"Converted: {input_path} -> {output_path}")
    except Exception as e:
        print(f"Error converting {input_path}: {e}")

# Iterate through all images in the input folder
for filename in os.listdir(input_folder):
    if filename.lower().endswith((".png", ".jpg", ".jpeg", ".bmp")):  # Supported extensions
        input_path = os.path.join(input_folder, filename)
        output_path = os.path.join(output_folder, os.path.splitext(filename)[0] + ".raw")
        convert_to_raw(input_path, output_path)

print("Conversion completed!")
