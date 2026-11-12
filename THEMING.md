# Renderer schemes

FrogUI keeps its existing drawing and input code. Each UI style loads its
built-in INI from `/mnt/sdcard/frogui/schemes/<style>.ini`; the shipped
`vertical.ini` documents the default layout and `Preview.ini` is a selectable
example. In Settings, **UI theme scheme**
selects an optional additional `<name>.ini` from that folder. The additional
scheme is applied over the style defaults. Missing or invalid keys retain the
previous value. Scheme colors override the selected FrogUI palette.

To add a theme, copy `Aurora.ini` into `frogui/schemes/` on the SD card. It will
appear in Settings after restarting FrogUI. Optional `background_image` artwork
goes in `frogui/schemes/Aurora/`; its font must be installed in
`frogui/fonts/`.

The loader accepts matching MuOS scheme fields below, case-insensitively:

| MuOS section/key | FrogUI effect |
| --- | --- |
| `[background] BACKGROUND` | screen background |
| `[header] HEADER_BACKGROUND`, `HEADER_TEXT`, `HEADER_HEIGHT` | background, title/accent color, header geometry |
| `[footer] FOOTER_BACKGROUND`, `FOOTER_TEXT`, `FOOTER_HEIGHT` | legend background/text and height |
| `[list] LIST_DEFAULT_BACKGROUND`, `LIST_DEFAULT_TEXT`, `LIST_DEFAULT_GLYPH_RECOLOUR` | background, normal text, folder color |
| `[list] LIST_FOCUS_BACKGROUND`, `LIST_FOCUS_TEXT`, `LIST_FOCUS_GLYPH_RECOLOUR`, `LIST_FOCUS_RADIUS` | selected item colors and corner radius |
| `[list] LIST_DISABLED_TEXT` | disabled item color |
| `[font] FONT_LIST_SIZE` | UI font size |
| `[misc] CONTENT_ITEM_COUNT`, `CONTENT_ITEM_HEIGHT`, `CONTENT_PADDING_LEFT`, `CONTENT_PADDING_TOP` | visible rows and list geometry |
| `[grid] COLUMN_COUNT`, `ROW_COUNT`, `COLUMN_PADDING`, `ROW_PADDING`, `CELL_RADIUS`, `CELL_IMAGE_PADDING_TOP` | browser grid layout |
| `[grid] CELL_DEFAULT_BACKGROUND`, `CELL_DEFAULT_TEXT`, `CELL_FOCUS_BACKGROUND`, `CELL_FOCUS_TEXT` | grid tile colors |

FrogUI-specific extensions are available in `[renderer]` (`row_height`,
`horizontal_padding`, `content_top`, `legend_height`, `selected_radius`,
`thumbnail_width`, `thumbnail_height`) and `[colours]` (`background`, `text`,
`selection`, `selection_text`, `header`, `folder`, `legend`,
`legend_background`, `disabled`). Colors use six-digit RGB hex values.

`[frogui] font_file` selects a font already installed under
`/mnt/sdcard/frogui/fonts/`. `[frogui] background_image` loads an image from
`/mnt/sdcard/frogui/schemes/<selected-scheme>/` (or the active style folder when
`Default` is selected). A picked user wallpaper still takes precedence. Paths
are limited to a filename to prevent escaping the theme folder.

The FrogUI extension `[frogui] home_layout=switch` enables the Switch launcher
layout. Its selected-state canvases live under
`schemes/<name>/640x480/image/static/muxlaunch/`, with the wall canvas at
`schemes/<name>/640x480/image/wall/default.png` and MuOS battery glyphs under
`schemes/<name>/glyph/header/`. The checked-in `Switch.ini` is configured as a
4×2 MuOS-style browser grid after entering Games. The canvas is aspect-fitted
for other panel sizes.

This is a renderer compatibility layer, not a `.muxthm` archive installer.
LVGL-only animation, opacity controls, MuOS `.bin` fonts, and unrelated MuOS
screens remain unsupported; the Switch scheme uses the active FrogUI TTF font.
Switch artwork is from the theme by Chronoss (original: PixelShift); see
`schemes/Switch/credits.txt`.
