#!/bin/bash
# Build App/Surprise/surprise inside the toolchain container. Extra args go to make.
set -e
cd "$(dirname "$0")/.."
IMAGE=onion-surprise-toolchain
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	docker build -t "$IMAGE" toolchain
fi
docker run --rm -v "$PWD":/root/workspace "$IMAGE" \
	/bin/bash -c 'source /root/.bashrc && make "$@"' make "$@"
