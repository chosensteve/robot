import numpy as np
from PIL import Image

# Small test map (so it’s easy to see)
rows, cols = 1171, 1143
pixel_map = [[0 for _ in range(cols)] for _ in range(rows)]

# Boundaries
for x in range(cols):
    pixel_map[0][x] = 1
    pixel_map[rows-1][x] = 1
for y in range(rows):
    pixel_map[y][0] = 1
    pixel_map[y][cols-1] = 1

# start zone (bottom-left corner)
for y in range(rows-600, rows):
    for x in range(320):
        pixel_map[y][x] = 2

# Cylinder sorting (right side, middle)
for y in range(20, 30):
    for x in range(cols-15, cols-5):
        pixel_map[y][x] = 3

# Cuboid sorting (top, middle)
for y in range(2, 10):
    for x in range(20, 30):
        pixel_map[y][x] = 4

# Convert to numpy
arr = np.array(pixel_map, dtype=np.uint8)

# Define color map
colors = {
    0: (255, 255, 255),  # free = white
    1: (0, 0, 0),        # boundary = black
    2: (0, 0, 255),      # pickup = blue
    3: (255, 0, 0),      # cylinder sorting = red
    4: (0, 255, 0)       # cuboid sorting = green
}

# Create RGB image
img = Image.new("RGB", (cols, rows))
pixels = img.load()
for y in range(rows):
    for x in range(cols):
        pixels[x, y] = colors[arr[y, x]]

img.save("field_map.png")