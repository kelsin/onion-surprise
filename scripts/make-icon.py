#!/usr/bin/env python3
"""Generate App/Surprise/surprise.png (74x74 RGBA gift box). Run: uv run --with pillow scripts/make-icon.py"""
import os
from PIL import Image, ImageDraw

S = 4  # supersample
img = Image.new("RGBA", (74 * S, 74 * S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
def r(x0, y0, x1, y1, c):
    d.rectangle([x0 * S, y0 * S, x1 * S - 1, y1 * S - 1], fill=c)
box, lid, rib, rib2 = (214, 64, 69, 255), (232, 90, 92, 255), (255, 214, 64, 255), (230, 180, 30, 255)
r(10, 34, 64, 66, box)       # body
r(6, 24, 68, 36, lid)        # lid
r(33, 34, 41, 66, rib)       # vertical ribbon
r(33, 24, 41, 36, rib2)
d.ellipse([20 * S, 8 * S, 38 * S, 26 * S], outline=rib, width=4 * S)   # bow loops
d.ellipse([36 * S, 8 * S, 54 * S, 26 * S], outline=rib, width=4 * S)
d.ellipse([32 * S, 18 * S, 42 * S, 28 * S], fill=rib2)
img = img.resize((74, 74), Image.LANCZOS)
img.save(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "App", "Surprise", "surprise.png"))
