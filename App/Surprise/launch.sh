#!/bin/sh
cd "$(dirname "$0")"
./surprise > log.txt 2>&1
exit 0
