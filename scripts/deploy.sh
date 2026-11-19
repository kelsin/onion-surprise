#!/bin/bash
# Usage: scripts/deploy.sh <host> [--run]
# Copies App/Surprise to the device over SSH (Tweaks -> Network; root, blank password).
set -e
cd "$(dirname "$0")/.."
HOST=${1:?usage: deploy.sh <host> [--run]}
ssh root@"$HOST" 'mkdir -p /mnt/SDCARD/App'
scp -r App/Surprise root@"$HOST":/mnt/SDCARD/App/
if [ "$2" = "--run" ]; then
	ssh root@"$HOST" 'cd /mnt/SDCARD/App/Surprise && chmod +x launch.sh surprise && ./launch.sh; cat log.txt'
fi
