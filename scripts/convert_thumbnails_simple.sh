#!/bin/bash
# Simple thumbnail converter using Python/Pillow
# Usage: ./convert_thumbnails_simple.sh [roms_directory]

ROMS_DIR="${1:-roms}"

if [ ! -d "$ROMS_DIR" ]; then
    echo "Error: ROMS directory '$ROMS_DIR' not found"
    echo "Usage: $0 [roms_directory]"
    exit 1
fi

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 not found!"
    echo "Please install Python 3"
    exit 1
fi

# Check if PIL/Pillow is installed
if ! python3 -c "from PIL import Image" 2>/dev/null; then
    echo "Error: Pillow (PIL) not found!"
    echo "Installing Pillow..."
    python3 -m pip install Pillow
    if [ $? -ne 0 ]; then
        echo "Failed to install Pillow"
        exit 1
    fi
fi

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Call the Python conversion script
python3 "$SCRIPT_DIR/convert_to_rgb565.py" "$ROMS_DIR"

if [ $? -ne 0 ]; then
    echo "Conversion failed!"
    exit 1
fi
