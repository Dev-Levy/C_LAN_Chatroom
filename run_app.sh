#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/build/char_device/" || exit 1

sudo insmod char_device.ko

cd "$SCRIPT_DIR/build/app/" || exit 1

sudo ./chat_app 192.168.110
