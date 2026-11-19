#!/bin/bash
# Usage: scripts/deploy.sh <host> [--run]
# Copies App/Surprise to the device over one SSH connection (Tweaks -> Network; root, blank password).
set -e
cd "$(dirname "$0")/.."
HOST=${1:?usage: deploy.sh <host> [--run]}
COPYFILE_DISABLE=1 tar -C App -cf - --exclude log.txt Surprise |
	ssh root@"$HOST" 'mkdir -p /mnt/SDCARD/App && tar -xf - -C /mnt/SDCARD/App'
if [ "$2" = "--run" ]; then
	echo "WARNING: MainUI is still running on the device. Press B only; pressing A stages a launch for the next MainUI exit." >&2
	ssh root@"$HOST" 'cd /mnt/SDCARD/App/Surprise && chmod +x launch.sh surprise && ./launch.sh; cat log.txt'
fi
