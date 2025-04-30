#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Change directory into build/char_device
cd "$SCRIPT_DIR/build/char_device/" || exit 1

sudo rmmod char_device.ko
# out one folder is char_device folder one 
cd ../..
# Change directory into the Makefiles folder
cd "$SCRIPT_DIR/Makefiles" || exit 1

make clean


