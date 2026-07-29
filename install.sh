#!/bin/bash

echo "Compiling and installing SmartFetch..."

gcc C_code/main.c C_code/ui.c -I./H_code -o sfetch

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi

sudo mkdir -p /usr/share/smartfetch/Ascii_art
sudo cp -r Ascii_art/* /usr/share/smartfetch/Ascii_art/

sudo mv sfetch /usr/local/bin/sfetch

echo "Installation successful!"
echo "You can now run 'sfetch' from any terminal."