#!/bin/bash
# Build (unless --no-build) and zip App/Surprise and Icons/Default/app/surprise.png into dist/Surprise-<version>.zip.
set -e
cd "$(dirname "$0")/.."
[ "$1" = "--no-build" ] || scripts/build.sh
BIN=App/Surprise/surprise
ICON=Icons/Default/app/surprise.png
[ -f "$ICON" ] || { echo "missing $ICON" >&2; exit 1; }
[ -f "$BIN" ] || { echo "missing $BIN" >&2; exit 1; }
file "$BIN" | grep -q 'ELF 32-bit.*ARM' || { echo "$BIN is not an ARM ELF: $(file "$BIN")" >&2; exit 1; }
VERSION=$(git describe --tags --always --dirty)
mkdir -p dist
OUT="$PWD/dist/Surprise-$VERSION.zip"
rm -f "$OUT"
zip -q -X -r "$OUT" App/Surprise "$ICON" -x 'App/Surprise/log.txt' -x '*.DS_Store'
unzip -l "$OUT" >&2
echo "$OUT"
