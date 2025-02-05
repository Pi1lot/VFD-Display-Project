from PIL import Image
import os

# Folder containing the images to be converted
input_folder = "icons"  # Change this name according to your source folder
output_folder = "images_bitmap"  # Converted images will be saved here
threshold = 128 # Contrast threshold for black to white conversion
# Create the output folder if it doesn't exist
if not os.path.exists(output_folder):
    os.makedirs(output_folder)


# Function to resize and convert an image to a monochrome bitmap
def resize_and_convert_to_monochrome(input_path, output_path):
    try:
        # Open the image
        with Image.open(input_path) as img:
            # Resize the image to 48x48
            img = img.resize((48, 48), Image.LANCZOS)
            # Convert to grayscale
            img = img.convert("L")  # L = Luminance (grayscale)
            # Convert to monochrome (binary)
            img = img.point(lambda x: 255 if x > threshold else 0, mode="1")  # Thresholding at 128

            # Check if the file already exists
            if not os.path.exists(output_path):
                # Save in BMP format
                img.save(output_path, format="BMP")
                print(f"Converted: {input_path} -> {output_path}")
            else:
                print(f"Existing file, conversion skipped: {output_path}")
    except Exception as e:
        print(f"Error converting {input_path}: {e}")


# Iterate through all images in the input folder
for filename in os.listdir(input_folder):
    if filename.lower().endswith((".png", ".jpg", ".jpeg", ".bmp", ".gif")):  # Supported extensions
        input_path = os.path.join(input_folder, filename)
        output_path = os.path.join(output_folder, os.path.splitext(filename)[0] + ".bmp")
        resize_and_convert_to_monochrome(input_path, output_path)

print("Conversion completed!")
