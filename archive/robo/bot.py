import numpy as np
from PIL import Image


rows, cols = 1171, 1143
pixel_map = [[0 for _ in range(cols)] for _ in range(rows)]


arr = np.array(pixel_map, dtype=np.uint8)
img = Image.fromarray(arr * 50)  # scale values so zones are visible
img.save("field_map.png")