# FrogOS Quick Start Guide

## What is FrogOS?

FrogOS is a file browser core for SF2000/GB300 multicore that lets you browse your ROMS folders and launch games directly. It's designed to mimic MinUI's clean interface.

## Quick Setup

### 1. Build the Core
```bash
cd /app
make CONSOLE=frogos CORE=cores/FrogOS
```

### 2. The core is automatically installed to:
- `sdcard/cores/frogos/core_87000000`

### 3. Launch FrogOS

To launch FrogOS, you need a stub file. The multicore system recognizes files in the format:
```
[corename];[filename].extension
```

For example, create a stub file:
```bash
# In any ROMS subfolder (e.g., /mnt/sda1/ROMS/gba/)
touch "frogos;browser.gba"
```

When multicore sees this file, it will load the `frogos` core.

## How to Use FrogOS

1. **Navigate**: Use D-Pad Up/Down to move through the list
2. **Enter Folders**: Press A to enter a folder
3. **Go Back**: Press B to go to the parent folder
4. **Launch Games**: Select a game file and press A to launch it

### Main Screen (ROMS Root)
- Shows only folders (gba, nes, snes, etc.)
- No files are shown at the root level
- Select a folder and press A to enter it

### Inside a Folder
- Shows subdirectories (if any) at the top
- Shows game files below
- ".." entry goes to parent folder

## Integration with Multicore Boot

### Method 1: Manual Launch
Place the stub file in any ROM folder and select it from the stock firmware's game list.

### Method 2: Auto-Boot on Startup (Advanced)
To make FrogOS launch automatically when the device boots:

1. Modify the main.c loader to check for a special trigger
2. Or create a default game selection that points to FrogOS

Example: Add this to main.c before the normal game loading:
```c
// Check if we should launch FrogOS by default
if (no_game_selected) {
    load_and_run_core("frogos;launcher", 0);
    return;
}
```

## Understanding the Format

The multicore system expects ROM files in this format:
```
corename;filename.extension
```

Examples:
- `gba;pokemon.gba` - Loads GBA core with pokemon ROM
- `nes;mario.gba` - Loads NES core with mario ROM (note: .gba extension is just for stock FW compatibility)
- `frogos;browser.gba` - Loads FrogOS core

## File Structure

```
/mnt/sda1/
├── ROMS/
│   ├── gba/
│   │   ├── pokemon.gba
│   │   └── frogos;browser.gba  <- Stub file to launch FrogOS
│   ├── nes/
│   └── snes/
├── cores/
│   ├── frogos/
│   │   └── core_87000000       <- FrogOS core binary
│   ├── gba/
│   └── nes/
└── frogos_boot.txt             <- Created when FrogOS launches a game
```

## Troubleshooting

### FrogOS won't start
- Make sure `core_87000000` exists in `/mnt/sda1/cores/frogos/`
- Verify the stub file name matches the pattern `frogos;[anything].[ext]`

### No folders show up
- Ensure your ROMS are in `/mnt/sda1/ROMS/`
- Check folder permissions

### Games won't launch
- The parent folder name must match a valid core name
- For example, games in `/mnt/sda1/ROMS/gba/` will try to launch with the `gba` core
- Make sure the target core exists in `/mnt/sda1/cores/[corename]/`

## Controls Reference

| Button | Action |
|--------|--------|
| D-Pad Up | Move selection up |
| D-Pad Down | Move selection down |
| A | Enter folder / Launch game |
| B | Go back to parent folder |

## What Happens When You Launch a Game?

1. FrogOS determines the core name from the parent folder (e.g., "gba", "nes")
2. It extracts the ROM path from the selected file
3. It writes this info to `/mnt/sda1/frogos_boot.txt` in format: `corename;/path/to/rom`
4. It triggers a shutdown via `RETRO_ENVIRONMENT_SHUTDOWN`
5. The multicore system reads `frogos_boot.txt` and launches the specified core with the ROM

## Building from Source

```bash
# Clean build
cd /app
make clean CONSOLE=frogos CORE=cores/FrogOS

# Build
make CONSOLE=frogos CORE=cores/FrogOS

# Or use the build script (add to buildcoresworking.sh)
echo "-- FrogOS make (File Browser) --"
make clean CONSOLE=frogos CORE=cores/FrogOS
make CONSOLE=frogos CORE=cores/FrogOS
```

## Next Steps

- Try launching FrogOS using the stub file
- Navigate your ROMS folders
- Launch a game and verify it works
- Consider integrating FrogOS as the default launcher for your multicore setup

Enjoy your new file browser! 🐸
