# FrogOS MinUI Design Implementation

FrogOS has been redesigned to match the clean, minimal aesthetic of MinUI.

## Design Features

### 🎨 Visual Style

**Color Scheme - Dark Theme:**
- Background: Very dark gray/black (#0841)
- Text: Light gray (#E71C)
- Selected background: Dark blue-gray (#2945)
- Selected text: White (#FFFF)
- Header: Medium gray (#39E7)
- Folders: Light color (#C618)

**Typography:**
- 16x16 pixel font (8x8 bitmap scaled 2x)
- Bold, clean appearance
- Excellent readability on small screens
- Proper character spacing (16px)

**Selection Indicator:**
- Pill-shaped rounded rectangles (8px corner radius)
- Fills width of screen with 4px margins
- Subtle, non-intrusive highlighting
- Matches MinUI's signature style

**Layout:**
- Clean header showing only current folder name
- "Systems" label for root directory
- No visual clutter or unnecessary text
- Generous padding (12px)
- 24px item height with proper spacing
- Scroll indicator on right side when needed

### 📐 Spacing & Padding

```
Header: 30px height
  ├─ Text at y=8 with 12px left padding
  └─ 8px gap before list

List Items: 24px each
  ├─ Selection box: 4px margin, 8px rounded corners
  ├─ Text: 12px left + 4px inner padding
  └─ 2px vertical padding

Scroll Bar: 2px wide, 6px from right edge
```

### 🎯 Key Differences from Original

| Aspect | Original | MinUI Style |
|--------|----------|-------------|
| **Font Size** | 8x8 | 16x16 (2x scaled) |
| **Selection** | Sharp rectangle | Rounded pill shape |
| **Colors** | Bright colors | Muted dark theme |
| **Header** | Full path + title | Just folder name |
| **Brackets** | [folder] markers | Clean names only |
| **Background** | Black | Dark gray |
| **Spacing** | Tight | Generous |

## MinUI Design Philosophy

FrogOS now follows MinUI's core principles:

1. **Minimal** - No unnecessary visual elements
2. **Clean** - Simple text, no clutter
3. **Focused** - Easy to read, easy to navigate
4. **Consistent** - Same style throughout
5. **Readable** - Large font, good contrast

## Technical Implementation

### Font Rendering
```c
// 8x8 font scaled 2x for 16x16 appearance
// Each pixel becomes a 2x2 block
for (int dy = 0; dy < 2; dy++) {
    for (int dx = 0; dx < 2; dx++) {
        framebuffer[py * SCREEN_WIDTH + px] = color;
    }
}
```

### Rounded Selection Box
```c
// Pill-shaped selection with 8px radius
draw_rounded_rect(x, y, width, height, 8, color);

// Uses circle approximation for smooth corners
// Draws center rectangles + rounded corners
```

### Layout Calculation
```c
int item_height = 24;  // Enough for 16px font + padding
int visible_entries = (SCREEN_HEIGHT - header - margin) / item_height;
int scroll_offset = selected_index - visible_entries / 2;
```

## Visual Comparison

**Before (Original FrogOS):**
- Small 8x8 font
- Sharp rectangular selection
- Bright white on black
- Full path display
- [Folder] brackets
- Tight spacing

**After (MinUI Style):**
- Large 16x16 font
- Smooth pill-shaped selection
- Muted colors, dark theme
- Simple folder name only
- No visual markers
- Generous spacing

## Screenshots Description

When you run FrogOS now, you'll see:

1. **Header**: Just "Systems" or the current folder name in medium gray
2. **Item List**: Clean folder/file names in light gray
3. **Selection**: The current item highlighted with a dark blue rounded pill
4. **Selected Text**: White text on the pill background
5. **Scroll Bar**: Thin gray bar on the right (if needed)
6. **Background**: Very dark, almost black

The overall feel is **calm, focused, and easy to read** - just like MinUI!

## Performance

- **Core Size**: 114KB (same as before, very efficient)
- **Rendering**: Fast, no lag
- **Memory**: Minimal overhead from rounded rect calculations
- **Compatibility**: Works on all SF2000/GB300 devices

## Customization

Want to tweak the design? Here are the key values in `frogos.c`:

```c
// Colors (line ~54)
#define COLOR_BG        0x0841  // Background
#define COLOR_SELECT_BG 0x2945  // Selection highlight

// Font scaling (line ~167)
for (int dy = 0; dy < 2; dy++)  // Change to 3 for even larger font

// Corner radius (line ~392)
int radius = 8;  // Increase for rounder, decrease for sharper

// Spacing (line ~365)
int item_height = 24;  // Increase for more spacing
int padding = 12;       // Adjust margins
```

## Credits

- **MinUI Design**: Shaun Inman (original creator)
- **FrogOS Implementation**: Based on MinUI's visual language
- **Font**: 8x8 public domain bitmap font, scaled 2x
- **Color Scheme**: Inspired by MinUI Dark theme

---

**FrogOS** - A MinUI-style file browser for SF2000/GB300 Multicore 🐸
