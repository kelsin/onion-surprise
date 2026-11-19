#!/usr/bin/env python3
"""Run make-surprise-json.py on a temp copy of the fixture card and validate the output format."""
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
KEYS = ["label", "rompath", "imgpath", "launch", "type"]

with tempfile.TemporaryDirectory() as d:
    card = os.path.join(d, "card")
    shutil.copytree(os.path.join(ROOT, "tests", "fixture-card"), card)
    os.remove(os.path.join(card, "Roms", "surprise.json"))
    subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "make-surprise-json.py"),
                    card, "-n", "6", "--seed", "1"], check=True)
    lines = open(os.path.join(card, "Roms", "surprise.json"), encoding="utf-8").read().splitlines()
    assert len(lines) == 6, lines
    for ln in lines:
        assert len(ln) < 1024
        o = json.loads(ln)
        assert list(o) == KEYS, o
        assert o["type"] == 5
        for k in KEYS[:4]:
            v = o[k]
            assert '"' not in v and ":" not in v.replace("/mnt/SDCARD", "", 1) if k != "label" else ('"' not in v and ":" not in v), v
            assert len(v.encode()) < 256
        assert o["rompath"].startswith("/mnt/SDCARD/Emu/") and "/../../Roms/" in o["rompath"]
        real = lambda p: os.path.join(card, p[len("/mnt/SDCARD/"):])
        assert os.path.isfile(real(o["rompath"])) and os.path.isfile(real(o["imgpath"]))
        assert os.path.isfile(real(o["launch"]))
    assert not any(json.loads(l)["label"] == "EarthBound" for l in lines)  # no box art
print("make-surprise-json: ok")
