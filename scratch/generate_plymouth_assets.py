#!/usr/bin/env python3
import os
import math
from PIL import Image, ImageDraw, ImageFont

ASSET_DIR = "airootfs/usr/share/plymouth/themes/archtitan-rise/assets"
os.makedirs(ASSET_DIR, exist_ok=True)

# 1. Generate dot.png (128x128 glowing circle)
img_dot = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
draw = ImageDraw.Draw(img_dot)
cx, cy = 64, 64
for r in range(60, 0, -1):
    alpha = int(255 * (1.0 - (r / 60.0) ** 1.8))
    # Gradient from cyan/violet blue glow to solid white center
    if r > 30:
        ratio = (r - 30) / 30.0
        red = int(255 * (1 - ratio) + 122 * ratio)
        green = int(255 * (1 - ratio) + 162 * ratio)
        blue = int(255 * (1 - ratio) + 247 * ratio)
    else:
        red, green, blue = 255, 255, 255
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(red, green, blue, alpha))
img_dot.save(os.path.join(ASSET_DIR, "dot.png"))

# 2. Generate dot_stretched.png (128x320 stretched streak)
img_stretch = Image.new("RGBA", (128, 320), (0, 0, 0, 0))
draw_s = ImageDraw.Draw(img_stretch)
for y in range(0, 320):
    # Oval stretching along vertical axis
    progress = y / 320.0
    width_at_y = 60 * math.sin(progress * math.pi)
    alpha = int(240 * math.sin(progress * math.pi))
    if width_at_y > 1:
        draw_s.line([(64 - width_at_y, y), (64 + width_at_y, y)], fill=(200, 220, 255, alpha), width=1)
img_stretch.save(os.path.join(ASSET_DIR, "dot_stretched.png"))

# 3. Generate Glyph PNGs
# Try system font or fallback to crisp vector rendered glyphs
font_candidates = [
    "/usr/share/fonts/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf"
]

font_path = None
for f in font_candidates:
    if os.path.exists(f):
        font_path = f
        break

font = ImageFont.truetype(font_path, 380) if font_path else ImageFont.load_default()

glyphs = {
    "R": "R",
    "i": "i",
    "s": "s",
    "e": "e",
    "comma": ",",
    "T": "T",
    "t": "t",
    "a": "a",
    "n": "n",
    "period": "."
}

for name, char in glyphs.items():
    # Calculate bounding box
    bbox = font.getbbox(char)
    w = max(60, bbox[2] - bbox[0] + 40)
    h = max(60, bbox[3] - bbox[1] + 40)
    
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    
    # Draw crisp white glyph
    d.text((-bbox[0] + 20, -bbox[1] + 20), char, font=font, fill=(240, 246, 255, 255))
    img.save(os.path.join(ASSET_DIR, f"glyph_{name}.png"))

print("Assets successfully generated in:", ASSET_DIR)
