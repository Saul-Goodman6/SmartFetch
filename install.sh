#!/bin/bash

echo "Compiling and installing SmartFetch..."

GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")

gcc C_code/main.c C_code/ui.c -I./H_code -DSF_VERSION=\"$GIT_HASH\" -o sfetch

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi

sudo mkdir -p /usr/share/smartfetch/Ascii_art
sudo cp -r Ascii_art/* /usr/share/smartfetch/Ascii_art/

sudo mv sfetch /usr/local/bin/sfetch

echo "Installation successful!"
echo "You can now run 'sfetch' from any terminal."