#!/usr/bin/env python3
"""For each theme in <themes-dir> with icons/app/dice.png drawn in one colour, write icons/app/surprise.png
(pixel-art gift in that colour). Run: uv run --with pillow scripts/make-theme-icons.py <themes-dir> [--dry-run]"""
import os
import sys
from PIL import Image

ART = """
..###....###..
.#...#..#...#.
.#....##....#.
..####..####..
##############
#.....##.....#
##############
.#....##....#.
.#....##....#.
.#....##....#.
.#....##....#.
.#....##....#.
.#....##....#.
.############.
""".split()
CELL, ORIGIN, SIZE = 4, 9, 74

def main():
    args = [a for a in sys.argv[1:] if a != "--dry-run"]
    dry = "--dry-run" in sys.argv[1:]
    if len(args) != 1:
        sys.exit("usage: make-theme-icons.py <themes-dir> [--dry-run]")
    root = args[0]
    for name in sorted(os.listdir(root)):
        theme = os.path.join(root, name)
        if not os.path.isdir(theme):
            continue
        dice = os.path.join(theme, "icons", "app", "dice.png")
        if not os.path.isfile(dice):
            print(f"{name}: skipped (no dice.png)")
            continue
        im = Image.open(dice).convert("RGBA")
        colours = {p[:3] for p in im.getdata() if p[3] > 128}
        if len(colours) != 1:
            print(f"{name}: skipped (not one colour: {len(colours)} colours)")
            continue
        c = colours.pop() + (255,)
        out = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
        for y, row in enumerate(ART):
            for x, ch in enumerate(row):
                if ch == "#":
                    for dy in range(CELL):
                        for dx in range(CELL):
                            out.putpixel((ORIGIN + x * CELL + dx, ORIGIN + y * CELL + dy), c)
        dest = os.path.join(theme, "icons", "app", "surprise.png")
        if not dry:
            out.save(dest)
        print(f"{name}: {'would write' if dry else 'written'} {dest}")

main()
