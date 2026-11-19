#!/usr/bin/env python3
"""Pick random games from a mounted Onion SD card and write Roms/surprise.json.

Usage: make-surprise-json.py <sd-card-root> [-n 10] [--seed S] [--dry-run]
"""
import argparse
import json
import os
import random
import re
import sys
import xml.etree.ElementTree as ET

DEVICE_ROOT = "/mnt/SDCARD"  # path strings always use this, whatever the local mount


def clean_label(s):
    s = s.replace('"', "").replace(":", "")
    s = re.sub(r"(\s*[\(\[][^\)\]]*[\)\]])+\s*$", "", s).strip()
    return s


def gamelist_names(romdir):
    names = {}
    try:
        root = ET.parse(os.path.join(romdir, "miyoogamelist.xml")).getroot()
    except (OSError, ET.ParseError):
        return names
    for g in root.iter("game"):
        p, n = g.findtext("path"), g.findtext("name")
        if p and n and n.strip():
            names[os.path.basename(p)] = n.strip()
    return names


def valid(*vals):
    """Mirror src/games.c value_ok: < 256 bytes, no '"', ':', '\\' or control chars."""
    return all(
        v and len(v.encode()) < 256 and not any(c in '":\\' or ord(c) < 0x20 for c in v)
        for v in vals
    )


def candidates(root):
    out = []
    for cfg in sorted(os.listdir(os.path.join(root, "Emu"))) if os.path.isdir(os.path.join(root, "Emu")) else []:
        emu = os.path.join(root, "Emu", cfg)
        try:
            with open(os.path.join(emu, "config.json"), encoding="utf-8") as f:
                c = json.load(f)
        except (OSError, ValueError):
            continue
        rp, ip, launch = c.get("rompath"), c.get("imgpath"), c.get("launch")
        if not (isinstance(rp, str) and isinstance(ip, str) and isinstance(launch, str) and rp and ip and launch):
            continue
        if "../../App/" in rp or os.path.normpath(os.path.join(emu, rp)).startswith(os.path.join(root, "App") + os.sep):
            continue
        romdir, imgdir = os.path.join(emu, rp), os.path.join(emu, ip)
        if not os.path.isdir(romdir):
            continue
        exts = {e.lower() for e in str(c.get("extlist", "")).split("|") if e}
        names = gamelist_names(romdir)
        for fn in sorted(os.listdir(romdir)):
            if fn.startswith(".") or not os.path.isfile(os.path.join(romdir, fn)):
                continue
            stem, ext = os.path.splitext(fn)
            if exts and ext[1:].lower() not in exts:
                continue
            if not os.path.isfile(os.path.join(imgdir, stem + ".png")):
                continue
            base = f"{DEVICE_ROOT}/Emu/{cfg}/"
            rom = f"{base}{rp.rstrip('/')}/{fn}"
            img = f"{base}{ip.rstrip('/')}/{stem}.png"
            lnch = f"{base}{launch}"
            label = clean_label(names.get(fn) or stem) or clean_label(stem)
            if not label or not valid(label, rom, img, lnch):
                continue
            entry = {"label": label, "rompath": rom, "imgpath": img, "launch": lnch, "type": 5}
            if len(json.dumps(entry, separators=(",", ":"), ensure_ascii=False).encode()) + 1 >= 1024:
                continue
            out.append((cfg, entry))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("root")
    ap.add_argument("-n", type=int, default=10)
    ap.add_argument("--seed")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()
    cands = candidates(a.root)
    if not cands:
        print("no games with box art found", file=sys.stderr)
        return 1
    rng = random.Random(a.seed)
    # spread across systems: shuffle each system's games, then round-robin
    by_sys = {}
    for s, e in cands:
        by_sys.setdefault(s, []).append(e)
    for v in by_sys.values():
        rng.shuffle(v)
    systems = sorted(by_sys)
    picked = []
    while len(picked) < a.n and any(by_sys.values()):
        rng.shuffle(systems)
        for s in systems:
            if by_sys[s] and len(picked) < a.n:
                picked.append(by_sys[s].pop())
    rng.shuffle(picked)
    lines = "".join(json.dumps(e, separators=(",", ":"), ensure_ascii=False) + "\n" for e in picked)
    if a.dry_run:
        sys.stdout.write(lines)
        return 0
    dest = os.path.join(a.root, "Roms", "surprise.json")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    tmp = dest + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(lines)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, dest)
    print(f"wrote {len(picked)} games to {dest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
