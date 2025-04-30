#!/bin/bash

sudo apt update

sudo apt install gcc

sudo apt-get install build-essential

sudo apt-get install linux-headers-$(uname -r)

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Change directory into the Makefiles folder
cd "$SCRIPT_DIR/Makefiles" || exit 1

# Now you're inside the Makefiles directory
pwd  # just to confirm where you are


make



# Change directory into build/char_device

cd "$SCRIPT_DIR/build/char_device/" || exit 1

sudo insmod char_device.ko

