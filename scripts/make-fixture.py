#!/usr/bin/env python3
"""Generate tests/fixture-card/: a fake SD card for host tests and the headless preview.

Run inside the preview image (has python3-pil) or with `uv run --with pillow scripts/make-fixture.py`.
"""
import json
import os
import shutil

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CARD = os.path.join(ROOT, "tests", "fixture-card")
FONT = os.path.join(ROOT, "App", "Surprise", "res", "font.ttf")
THEME = os.path.join(CARD, "Themes", "Fixture")

# (system, rom file, label, art size or None, art colour)
GAMES = [
    ("GBA", "Advance Wars (USA).gba", "Advance Wars", (500, 700), (200, 60, 50)),
    ("GBA", "Golden Sun (USA).gba", "Golden Sun", (250, 360), (210, 160, 40)),
    ("SNES", "Chrono Trigger (USA).sfc", "Chrono Trigger", (640, 480), (40, 110, 190)),
    ("GBA", "Mother 3 (Japan) (Translated En).gba",
     "Mother 3 (Japan) (Translated En) Extra Long Title Edition", (300, 420), (120, 70, 160)),
    ("SNES", "EarthBound (USA).sfc", "EarthBound", None, None),
    ("SNES", "Super Metroid (Japan, USA).sfc", "Super Metroid", (180, 260), (60, 150, 90)),
    ("GBA", "Metroid Fusion (USA).gba", "Metroid Fusion", (256, 380), (40, 160, 170)),
    ("SNES", "Donkey Kong Country 2 (USA).sfc", "Donkey Kong Country 2 - Diddy's Kong Quest",
     (400, 560), (150, 90, 40)),
    ("SNES", "Final Fantasy VI (USA).sfc", "Final Fantasy VI", (250, 350), (90, 90, 110)),
    ("GBA", "Pokemon Emerald (USA).gba", "Pokemon Emerald", (250, 360), (40, 140, 70)),
]


def font(size):
    return ImageFont.truetype(FONT, size)


def write(rel, data):
    p = os.path.join(CARD, rel)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w") as f:
        f.write(data)
    return p


def save(img, rel):
    p = os.path.join(CARD, rel)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    img.save(p, optimize=True)


def vgradient(w, h, top, bottom, alpha=255):
    img = Image.new("RGBA", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        c = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
        d.line([(0, y), (w, y)], fill=c + (alpha,))
    return img


def make_theme():
    write("system.json", json.dumps({"theme": "/mnt/SDCARD/Themes/Fixture/"}, indent=1) + "\n")
    write("Themes/Fixture/config.json", json.dumps({
        "name": "Fixture",
        "title": {"size": 30, "color": "#FFFFFF"},
        "list": {"size": 26, "color": "#E8E8F0"},
        "hint": {"size": 22, "color": "#C8C8D8"},
    }, indent=1) + "\n")
    save(vgradient(640, 480, (22, 26, 44), (10, 10, 18)), "Themes/Fixture/skin/background.png")
    save(vgradient(640, 60, (70, 90, 150), (40, 55, 100), 200), "Themes/Fixture/skin/bg-list-s.png")
    save(Image.new("RGBA", (640, 4), (255, 255, 255, 28)), "Themes/Fixture/skin/div-line-h.png")
    save(Image.new("RGBA", (640, 60), (0, 0, 0, 120)), "Themes/Fixture/skin/tips-bar-bg.png")
    panel = Image.new("RGBA", (256, 364), (0, 0, 0, 0))
    ImageDraw.Draw(panel).rounded_rectangle([0, 0, 255, 363], 12, fill=(0, 0, 0, 90))
    save(panel, "Themes/Fixture/skin/preview-bg.png")
    for letter, colour in (("A", (200, 60, 60)), ("B", (220, 180, 40))):
        icon = Image.new("RGBA", (54, 54), (0, 0, 0, 0))
        d = ImageDraw.Draw(icon)
        d.ellipse([7, 7, 46, 46], fill=colour + (255,))
        d.text((27, 27), letter, font=font(24), fill=(255, 255, 255, 255), anchor="mm")
        save(icon, "Themes/Fixture/skin/icon-%s-54.png" % letter)


def make_art(label, size, colour):
    w, h = size
    dark = tuple(c // 3 for c in colour)
    img = vgradient(w, h, colour, dark).convert("RGB")
    d = ImageDraw.Draw(img)
    d.rectangle([4, 4, w - 5, h - 5], outline=(255, 255, 255), width=3)
    words, lines = label.split(), []
    for word in words:
        if lines and len(lines[-1]) + len(word) < 12:
            lines[-1] += " " + word
        else:
            lines.append(word)
    fs = max(14, w // 10)
    for i, line in enumerate(lines[:4]):
        d.text((w // 2, h // 3 + i * (fs + 6)), line, font=font(fs), fill=(255, 255, 255), anchor="mm")
    d.text((w // 2, h - fs), "%dx%d" % size, font=font(fs * 2 // 3), fill=(230, 230, 230), anchor="mm")
    return img


def make_games():
    lines = []
    for system in sorted({g[0] for g in GAMES}):
        write("Emu/%s/launch.sh" % system, "#!/bin/sh\necho fake %s emulator \"$1\"\n" % system)
        os.chmod(os.path.join(CARD, "Emu", system, "launch.sh"), 0o755)
        write("Emu/%s/config.json" % system, json.dumps({
            "label": system, "icon": "", "launch": "launch.sh",
            "rompath": "../../Roms/%s" % system, "imgpath": "../../Roms/%s/Imgs" % system,
            "extlist": "gba|zip" if system == "GBA" else "sfc|smc|zip",
        }, indent=1) + "\n")
    for system, rom, label, size, colour in GAMES:
        write("Roms/%s/%s" % (system, rom), "")
        base = "/mnt/SDCARD/Emu/%s/../../Roms/%s/" % (system, system)
        entry = {"label": label, "rompath": base + rom}
        if size:
            stem = os.path.splitext(rom)[0]
            save(make_art(label, size, colour), "Roms/%s/Imgs/%s.png" % (system, stem))
            entry["imgpath"] = base + "Imgs/%s.png" % stem
        entry["launch"] = "/mnt/SDCARD/Emu/%s/launch.sh" % system
        entry["type"] = 5
        lines.append(json.dumps(entry, separators=(",", ":")))
    write("Roms/surprise.json", "\n".join(lines) + "\n")


if __name__ == "__main__":
    shutil.rmtree(CARD, ignore_errors=True)
    make_theme()
    make_games()
    print("wrote", os.path.relpath(CARD, ROOT))
