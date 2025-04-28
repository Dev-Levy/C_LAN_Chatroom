#!/bin/sh
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"


# Change directory into build/char_device
cd "$SCRIPT_DIR/build/char_device/" || exit 1

sudo rmmod char_dev.ko


# Change directory into the Makefiles folder
cd "$SCRIPT_DIR/Makefiles" || exit 1

make clean


