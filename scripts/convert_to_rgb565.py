#!/usr/bin/env python3
"""
Convert PNG thumbnails to raw RGB565 format for FrogOS/SF2000
Usage: python convert_to_rgb565.py <roms_directory>
"""

import os
import sys
import struct
from pathlib import Path
from PIL import Image

def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB values to 16-bit RGB565 format"""
    # RGB565: RRRRR GGGGGG BBBBB (5 bits R, 6 bits G, 5 bits B)
    r5 = (r >> 3) & 0x1F  # 5 bits
    g6 = (g >> 2) & 0x3F  # 6 bits
    b5 = (b >> 3) & 0x1F  # 5 bits
    return (r5 << 11) | (g6 << 5) | b5

def convert_image_to_rgb565(input_path, output_path, max_width, max_height):
    """Convert a PNG image to raw RGB565 format"""
    try:
        # Open and convert image to RGB
        img = Image.open(input_path)
        img = img.convert('RGB')

        # Resize if needed, maintaining aspect ratio
        img.thumbnail((max_width, max_height), Image.Resampling.LANCZOS)

        # Get pixel data
        pixels = img.load()
        width, height = img.size

        # Convert to RGB565 and write
        with open(output_path, 'wb') as f:
            for y in range(height):
                for x in range(width):
                    r, g, b = pixels[x, y]
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    # Write as little-endian uint16
                    f.write(struct.pack('<H', rgb565))

        return True, width, height
    except Exception as e:
        return False, 0, 0

def main():
    if len(sys.argv) < 2:
        print("Usage: python convert_to_rgb565.py <roms_directory>")
        sys.exit(1)

    roms_dir = Path(sys.argv[1])

    if not roms_dir.exists():
        print(f"Error: Directory '{roms_dir}' not found")
        sys.exit(1)

    print(f"Converting PNG thumbnails to RGB565 format...")
    print(f"Scanning: {roms_dir}")

    count = 0
    converted = 0
    errors = 0

    # Find all PNG files in .res subdirectories
    for png_file in roms_dir.rglob('*.png'):
        if '.res' in png_file.parts:
            count += 1
            rgb565_file = png_file.with_suffix('.rgb565')

            print(f"Converting: {png_file.name}")

            # Determine size based on image aspect ratio
            try:
                with Image.open(png_file) as img:
                    width, height = img.size

                if width > height:
                    # Wide image - 250x200 max
                    max_w, max_h = 250, 200
                    print(f"  Wide image ({width}x{height}): resizing to {max_w}x{max_h} max")
                else:
                    # Square/vertical - 200x200 max
                    max_w, max_h = 200, 200
                    print(f"  Square/vertical ({width}x{height}): resizing to {max_w}x{max_h} max")

                success, final_w, final_h = convert_image_to_rgb565(
                    png_file, rgb565_file, max_w, max_h
                )

                if success:
                    converted += 1
                    file_size = rgb565_file.stat().st_size
                    print(f"  Success: {final_w}x{final_h} ({file_size} bytes)")
                else:
                    errors += 1
                    print(f"  Error converting {png_file}")

            except Exception as e:
                errors += 1
                print(f"  Error: {e}")

    print()
    print("Conversion complete!")
    print(f"Found: {count} PNG files")
    print(f"Converted: {converted} files")
    print(f"Errors: {errors} files")
    print()
    print("RGB565 files created - ready for SF2000!")

if __name__ == '__main__':
    main()
