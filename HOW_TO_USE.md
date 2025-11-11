# FrogUI - How to Use

FrogUI is a modern file browser interface for the SF2000/GB300 that provides access to multiple emulator cores with thumbnails, recent games, and customizable themes.

## Basic Navigation

- **D-Pad**: Navigate through files and folders
- **A Button**: Select/launch items, confirm in menus
- **B Button**: Go back, cancel in menus  
- **SELECT**: Access per-core settings (in console folders)
- **START**: Access main settings
- **L/R Buttons**: Navigate between sections (Recent, Tools, etc.)

## Adding ROMs

1. **Create Console Folders**: Place ROMs in `/ROMS/[console]/` folders on your SD card:
   ```
   /ROMS/gb/          - Game Boy games
   /ROMS/gba/         - Game Boy Advance games  
   /ROMS/nes/         - NES/Famicom games
   /ROMS/snes/        - SNES games
   /ROMS/md/          - Mega Drive/Genesis games
   ```

2. **Supported Formats**: Each core supports different formats:
   - **GB**: `.gb`, `.gbc` files
   - **GBA**: `.gba` files
   - **NES**: `.nes` files  
   - **SNES**: `.sfc`, `.smc` files
   - **Genesis**: `.md`, `.gen` files

3. **Console Mapping**: FrogUI automatically maps console folders to cores:
   - `gb` → Gambatte (Game Boy)
   - `gba` → gpSP (Game Boy Advance)  
   - `nes` → QuickNES (Nintendo Entertainment System)
   - `snes` → Snes9x (Super Nintendo)
   - And 60+ more core mappings

## Adding Thumbnails

Thumbnails enhance your gaming experience by showing preview images of your games.

### Thumbnail Requirements

- **Format**: RGB565 raw format (no headers)
- **Supported Resolutions**: 
  - 64×64 pixels (8,192 bytes)
  - 128×128 pixels (32,768 bytes) 
  - 160×160 pixels (51,200 bytes) - **Recommended**
  - 200×200 pixels (80,000 bytes)
  - 250×200 pixels (100,000 bytes)

### Thumbnail Placement

Place thumbnail files alongside your ROMs with `.rgb565` extension:
```
/ROMS/nes/Super Mario Bros.nes
/ROMS/nes/Super Mario Bros.rgb565

/ROMS/gb/Tetris.gb
/ROMS/gb/Tetris.rgb565
```

### Converting Thumbnails

FrogUI includes automatic conversion scripts in the `scripts/` folder:

**Windows Users:**
```batch
scripts\convert_thumbnails_simple.bat D:\roms
```

**Linux/Mac Users:**
```bash
./scripts/convert_thumbnails_simple.sh /path/to/roms
```

The scripts will:
1. Find all PNG images in `.res` subdirectories
2. Automatically resize them (200×200 for square images, 250×200 for wide)
3. Convert to RGB565 raw format
4. Save with `.rgb565` extension

**Requirements:**
- Python 3 with Pillow (PIL) library
- PNG source images in `.res` folders alongside your ROMs

**Manual Conversion:**
If you prefer manual conversion:
1. Resize image to 160×160 pixels (or 200×200/250×200)
2. Convert to RGB565 raw format (5 bits R, 6 bits G, 5 bits B)
3. Save as raw binary file with `.rgb565` extension

## Settings and Configuration

### Main Settings (START Button)

Access global multicore settings:
- **Tearing Fix**: Adjust display sync
- **RGB Clock**: Display timing settings
- **Screen Scaling**: Choose scaling modes
- **Theme**: Select from 19 beautiful themes
- **Show FPS**: Toggle FPS display

### Per-Core Settings (SELECT Button)

When in a console folder, press SELECT to access core-specific settings:
- Each core has its own configuration file
- Settings are saved to `/configs/[CoreName]/[CoreName].opt`
- Examples: `/configs/Gambatte/Gambatte.opt`, `/configs/QuickNES/QuickNES.opt`

### Available Themes

Choose from 19 carefully crafted themes:

**Modern Themes:**
- MinUI Style (classic black/white)
- Emerald, Orange, Golden, Rose
- Purple, Prosty's Pink, Green, Red

**Retro Themes:**
- Commodore 64 (blue/purple classic)
- Game Boy (classic green monochrome)
- NES (gray/red Nintendo colors)
- Famicom (red/yellow Japanese colors)
- SNES (purple/gray Super Nintendo)

**CRT/Terminal Themes:**
- Amber CRT (orange on black)
- Green CRT (green on black) 
- DOS (blue background, white text)
- Matrix (green digital rain style)

## Tools and Utilities

### Tools Menu (L/R to navigate)

Access various system tools:

**Recent Games**: Quick access to recently played games
- Automatically tracks your last 10 games
- Shows game name and console
- Press A to launch directly

**Tools**: System utilities and core management
- Access to various system tools
- Core-specific utilities

**Utils**: JavaScript utilities (js2000 core)
- Run `.js` files from `/ROMS/js2000/` folder  
- JavaScript utilities for system management
- Configuration scripts and tools

### Folder Navigation Shortcuts

- **Boundary Delay**: When scrolling to the beginning or end of long file lists, FrogUI pauses briefly to prevent accidental wrap-around
- **Alphabetical Sorting**: All folders and files are sorted alphabetically
- **Folder Filtering**: System folders (frogui, js2000) are hidden from ROM lists

## Tips and Tricks

1. **Organize by Console**: Keep ROMs organized in console-specific folders for best experience

2. **Use Thumbnails**: 160×160 RGB565 thumbnails provide the best quality-to-size ratio

3. **Recent Games**: Your most-played games appear in the Recent section for quick access

4. **Theme Switching**: Experiment with different themes to find your preferred style

5. **Per-Core Settings**: Each emulator core can have different settings optimized for that system

6. **Utils Section**: Explore the JavaScript utilities for advanced system management

## File Structure Overview

```
/ROMS/
├── gb/           - Game Boy ROMs
├── gba/          - Game Boy Advance ROMs  
├── nes/          - NES ROMs
├── snes/         - SNES ROMs
├── js2000/       - JavaScript utilities
└── [console]/    - Other console ROMs

/configs/
├── multicore.opt     - Main FrogUI settings
├── Gambatte/         - Game Boy core settings
├── QuickNES/         - NES core settings  
└── [CoreName]/       - Other core settings

/bios/
└── bisrv.asd         - FrogUI firmware
```

## Troubleshooting

**Problem**: ROM doesn't launch
- **Solution**: Check ROM is in correct console folder with proper file extension

**Problem**: No thumbnail showing  
- **Solution**: Ensure thumbnail is RGB565 format and matches ROM filename exactly

**Problem**: Settings not saving
- **Solution**: Make sure SD card is not write-protected

**Problem**: Core not working
- **Solution**: Check if core is included in your FrogUI build (see buildcoresworking.sh)

Enjoy your enhanced gaming experience with FrogUI!