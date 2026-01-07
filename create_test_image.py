import numpy as np
from PIL import Image

# Create a 2x2 test image with 80% luminance in top-right pixel
# This should give us: top-left=5%, top-right=80%, bottom-left=5%, bottom-right=10%
width, height = 2, 2
img = np.zeros((height, width, 3), dtype=np.float32)

# Top-left (0,0): 5% brightness
img[0, 0] = [0.05, 0.05, 0.05]

# Top-right (0,1): 80% brightness
img[0, 1] = [0.8, 0.8, 0.8]

# Bottom-left (1,0): 5% brightness
img[1, 0] = [0.05, 0.05, 0.05]

# Bottom-right (1,1): 10% brightness
img[1, 1] = [0.1, 0.1, 0.1]

# Convert to 8-bit for PNG (0-255)
img_8bit = (img * 255).astype(np.uint8)

# Save as PNG
Image.fromarray(img_8bit).save('mipmap_test_2x2.png')
print("Created mipmap_test_2x2.png")
print("Total luminance:", img.sum())
print("Pixel luminances:", img[:,:,0])
