#!/bin/sh
set -e -x

# Generate all of the PNGs
convert -background none icon.svg -resize 512x512 icon512.png
convert icon512.png -resize 256x256 icon256.png
convert icon512.png -resize 128x128 icon128.png
convert icon512.png -resize 32x32 icon032.png
convert icon512.png -resize 16x16 icon016.png

# Build the Mac icon
png2icns ../darwin/erizo.icns icon*.png

# Build the Windows icon
convert icon*.png ../win64/erizo.ico

# Clean up
rm icon512.png icon256.png icon128.png icon032.png icon016.png
