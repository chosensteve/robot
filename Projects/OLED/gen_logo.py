import urllib.request
from PIL import Image
import io

def generate_adafruit_bitmap():
    # Download official high res logo
    url = "https://upload.wikimedia.org/wikipedia/commons/thumb/1/19/Spotify_logo_without_text.svg/1024px-Spotify_logo_without_text.svg.png"
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla'})
    response = urllib.request.urlopen(req)
    
    # Needs to be solid black/white
    img = Image.open(io.BytesIO(response.read())).convert("RGBA")
    
    # Paste on black background 
    bg = Image.new("RGBA", img.size, (0, 0, 0, 255))
    img = Image.alpha_composite(bg, img)
    # The logo is green (RGB 30, 215, 96). We want it to be white on OLED.
    # Convert to grayscale
    img_gray = img.convert("L")
    
    # Scale exactly to 32x32 using anti-aliasing
    img_small = img_gray.resize((32, 32), Image.Resampling.LANCZOS)
    
    # Threshold it. We want the green logo parts to become 1 (white on OLED). 
    # Green in grayscale is ~150.
    img_bin = img_small.point(lambda p: 255 if p > 50 else 0, mode='1')
    
    # Ensure it's 32x32
    width, height = img_bin.size
    pixels = img_bin.load()
    
    array_lines = []
    
    for y in range(height):
        byte_strs = []
        for xi in range(width // 8):
            byte_val = 0
            for b in range(8):
                # If pixel is >0 (the logo part), bit is 1
                if pixels[xi * 8 + b, y] > 0:
                    byte_val |= (1 << (7 - b))
            byte_strs.append(f"0x{byte_val:02x}")
        array_lines.append("\t" + ", ".join(byte_strs))
        
    code = "const unsigned char PROGMEM spotify_logo [] = {\n" + ",\n".join(array_lines) + "\n};"
    
    with open("logo.txt", "w") as f:
        f.write(code)

if __name__ == "__main__":
    generate_adafruit_bitmap()
