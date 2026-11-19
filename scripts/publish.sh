#!/bin/bash
# Copy a GitHub release's Surprise-vX.Y.Z.zip into storage (default /Volumes/roms/os/onion).
# Usage: scripts/publish.sh [--dry-run] [vX.Y.Z]   (no tag = latest release)
# Never overwrites an existing zip, never deletes anything, writes nothing else to storage.
set -euo pipefail
cd "$(dirname "$0")/.."
REPO=kelsin/onion-surprise
DEST=${SURPRISE_STORAGE:-/Volumes/roms/os/onion}
DRY=0
[ "${1:-}" = "--dry-run" ] && { DRY=1; shift; }
TAG=${1:-$(gh release view --repo "$REPO" --json tagName -q .tagName)}
[[ "$TAG" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "not a release tag: $TAG" >&2; exit 1; }
NAME="Surprise-$TAG.zip"
[ -d "$DEST" ] || { echo "storage not mounted: $DEST" >&2; exit 1; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
gh release download "$TAG" --repo "$REPO" --pattern "$NAME" --dir "$TMP"
ZIP="$TMP/$NAME"

# Layout check: App/Surprise/ (with binary and config) plus the default icon, nothing else.
FILES=$(unzip -Z1 "$ZIP")
BAD=$(grep -v -e '^App/Surprise/' -e '^Icons/$' -e '^Icons/Default/$' -e '^Icons/Default/app/$' \
	-e '^Icons/Default/app/surprise.png$' <<<"$FILES" || true)
[ -z "$BAD" ] || { echo "unexpected entries in $NAME:" >&2; echo "$BAD" >&2; exit 1; }
for f in App/Surprise/surprise App/Surprise/config.json; do
	grep -qx "$f" <<<"$FILES" || { echo "$NAME is missing $f" >&2; exit 1; }
done

if [ -e "$DEST/$NAME" ]; then
	cmp -s "$ZIP" "$DEST/$NAME" && { echo "already published: $DEST/$NAME"; exit 0; }
	echo "$DEST/$NAME exists and differs; not overwriting" >&2
	exit 1
fi
if [ "$DRY" = 1 ]; then
	echo "dry run: would copy $NAME ($(wc -c <"$ZIP" | tr -d ' ') bytes) to $DEST/"
	exit 0
fi
# Copy under a hidden name, then rename, so a partial copy never looks like a release.
# cat (not cp) so no extended attributes or ._ files reach the share.
PART="$DEST/.$NAME.partial"
trap 'rm -rf "$TMP"; rm -f "$PART"' EXIT
cat "$ZIP" >"$PART"
cmp -s "$ZIP" "$PART" || { echo "copy verification failed" >&2; exit 1; }
mv -n "$PART" "$DEST/$NAME"
[ -e "$PART" ] && { echo "$DEST/$NAME appeared during copy; not overwriting" >&2; exit 1; }
echo "published $DEST/$NAME"
