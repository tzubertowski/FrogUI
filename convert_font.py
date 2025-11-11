#!/usr/bin/env python3
"""
Convert ChillRoundM.ttf to bitmap C array for FrogOS
Generates 16x16 pixel bitmaps for ASCII 32-127 (96 characters)
"""

from PIL import Image, ImageDraw, ImageFont
import sys

def generate_font_bitmap(ttf_path, font_size, output_path):
    """Generate bitmap font from TTF file"""

    # Load the font at 4x size for better quality
    try:
        font_large = ImageFont.truetype(ttf_path, font_size * 4)
    except Exception as e:
        print(f"Error loading font: {e}")
        sys.exit(1)

    # ASCII characters 32-127 (96 characters)
    chars = [chr(i) for i in range(32, 128)]

    # Character dimensions
    char_width = 16
    char_height = 16
    large_size = 64  # 4x larger

    # Generate C array
    c_array = []
    c_array.append("// GamePocket font - 16x16 bitmap (96 characters, ASCII 32-127)")
    c_array.append("static const uint8_t font_gamepocket_16x16[96][32] = {")

    # Position baseline lower to give room for ascenders at the top
    # baseline_y at 75% of large_size gives room for tall letters
    baseline_y = int(large_size * 0.75)

    for idx, char in enumerate(chars):
        # Create image for this character
        img_large = Image.new('L', (large_size, large_size), color=0)
        draw = ImageDraw.Draw(img_large)

        # Draw character at the SAME baseline position
        # anchor='ls' means "left, baseline" - consistent for all characters
        # NO stroke - ZenMaru Bold is already bold enough
        draw.text((large_size//2, baseline_y), char, font=font_large, anchor='ls', fill=255)

        # Scale down to 16x16 with LANCZOS for smooth antialiasing
        img = img_large.resize((char_width, char_height), Image.Resampling.LANCZOS)

        # Convert to bitmap (threshold at 100 for bolder result)
        pixels = img.load()

        # Store as 16 rows of 2 bytes each (16 bits per row)
        bytes_data = []
        for row in range(char_height):
            row_bits = 0
            for col in range(char_width):
                if pixels[col, row] > 50:  # Even lower threshold for maximum boldness
                    row_bits |= (1 << (15 - col))

            # Split into 2 bytes (big-endian)
            byte1 = (row_bits >> 8) & 0xFF
            byte2 = row_bits & 0xFF
            bytes_data.append(byte1)
            bytes_data.append(byte2)

        # Format as C array
        char_repr = repr(char) if char != '\\' else "'\\\\'"
        hex_values = ','.join([f'0x{b:02x}' for b in bytes_data])
        c_array.append(f"    {{{hex_values}}}, // {char_repr} (ASCII {ord(char)})")

    c_array.append("};")

    # Write to file
    with open(output_path, 'w') as f:
        f.write('\n'.join(c_array))

    print(f"Font bitmap generated: {output_path}")
    print(f"Characters: {len(chars)}")
    print(f"Size: 16x16 pixels per character")
    print(f"Array size: {len(chars)} x 32 bytes = {len(chars) * 32} bytes")

if __name__ == "__main__":
    ttf_path = "/app/cores/FrogOS/GamePocket-Regular-ZeroKern.ttf"  # Gaming font!
    output_path = "/app/cores/FrogOS/font_gamepocket.h"
    font_size = 14  # TTF point size (will be rendered at 4x then downscaled) - smaller for GamePocket

    generate_font_bitmap(ttf_path, font_size, output_path)
