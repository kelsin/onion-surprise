#!/bin/bash
# Build the app natively and render headless screenshots of the fixture card into build/preview/.
set -e
cd "$(dirname "$0")/.."
IMAGE=onion-surprise-preview
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	docker build -t "$IMAGE" -f toolchain/preview.Dockerfile toolchain
fi
docker run --rm -v "$PWD":/work "$IMAGE" /bin/bash -euc '
make preview-bin
out=build/preview
tmp=$(mktemp -d)
shot() { # name card [args...]
	local name=$1 card=$2; shift 2
	SDL_VIDEODRIVER=dummy SURPRISE_SDCARD=$card SURPRISE_TMP=$tmp \
		$out/surprise --screenshot $out/$name.bmp "$@"
	python3 -c "import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[2])" \
		$out/$name.bmp $out/$name.png
	rm $out/$name.bmp
	echo "wrote $out/$name.png"
}
shot list tests/fixture-card --select 1
shot list-noart tests/fixture-card --select 4
shot list-scrolled tests/fixture-card --select 8
mkdir -p $tmp/empty-card && cp tests/fixture-card/system.json $tmp/empty-card/
ln -s "$PWD/tests/fixture-card/Themes" $tmp/empty-card/Themes
shot empty $tmp/empty-card
mkdir -p $tmp/bare-card # no system.json / theme: solid-colour + bundled-font fallbacks
ln -s "$PWD/tests/fixture-card/Emu" "$PWD/tests/fixture-card/Roms" $tmp/bare-card/
shot list-notheme $tmp/bare-card --select 1
'
