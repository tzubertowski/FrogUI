# Renderer schemes

The **UI theme** setting selects TreeFrogUI, ProMinimal, or an installed scheme.
L2 cycles the Explore Content root layout for every theme: Vertical, Horizontal,
then System View. Select remains the core picker. MinUIfied starts in Vertical.
The layout choice is saved independently from the selected theme. TreeFrogUI
retains the `vertical` settings key and `Vertical.json` profile name for existing
installs. Installed `<name>.ini` profiles are loaded over Vertical defaults;
missing or invalid keys keep the default. Legacy `style` and `renderer_scheme`
settings are read when migrating older settings files, then saved as one
`ui_theme` value.

To add a theme, copy `Aurora.ini` into `frogui/schemes/` on the SD card. It will
appear in Settings after restarting FrogUI. Optional `background_image` artwork
goes in `frogui/schemes/Aurora/`; its font must be installed in
`frogui/fonts/`.

The loader accepts matching MuOS scheme fields below, case-insensitively:

| MuOS section/key | FrogUI effect |
| --- | --- |
| `[background] BACKGROUND` | screen background |
| `[header] HEADER_BACKGROUND`, `HEADER_TEXT`, `HEADER_HEIGHT` | header background, title/accent color, header geometry |
| `[footer] FOOTER_BACKGROUND`, `FOOTER_TEXT`, `FOOTER_HEIGHT` | legend background/text and height |
| `[list] LIST_DEFAULT_BACKGROUND`, `LIST_DEFAULT_TEXT`, `LIST_DEFAULT_GLYPH_RECOLOUR` | background, normal text, folder color |
| `[list] LIST_FOCUS_BACKGROUND`, `LIST_FOCUS_TEXT`, `LIST_FOCUS_GLYPH_RECOLOUR`, `LIST_FOCUS_RADIUS` | selected item colors and corner radius |
| `[list] LIST_DISABLED_TEXT` | disabled item color |
| `[font] FONT_LIST_SIZE` | UI font size |
| `[misc] CONTENT_ITEM_COUNT`, `CONTENT_ITEM_HEIGHT`, `CONTENT_PADDING_LEFT`, `CONTENT_PADDING_TOP`, `CONTENT_WIDTH`, `CONTENT_HEIGHT`, `CONTENT_ALIGNMENT` | visible rows and list geometry |
| `[grid] COLUMN_COUNT`, `ROW_COUNT`, `COLUMN_PADDING`, `ROW_PADDING`, `CELL_RADIUS`, `CELL_IMAGE_PADDING_TOP` | browser grid layout |
| `[grid] CELL_DEFAULT_BACKGROUND`, `CELL_DEFAULT_TEXT`, `CELL_FOCUS_BACKGROUND`, `CELL_FOCUS_TEXT` | grid tile colors |

FrogUI-specific extensions are available in `[renderer]` (`row_height`,
`horizontal_padding`, `content_top`, `content_width`, `content_height`,
`content_alignment`, `list_padding_left`, `list_padding_top`,
`list_padding_bottom`, `legend_height`, `selected_radius`,
`thumbnail_width`, `thumbnail_height`) and `[colours]` (`background`, `text`,
`selection`, `selection_text`, `header`, `folder`, `legend`,
`legend_background`, `disabled`). Colors use six-digit RGB hex values.

`[frogui] font_file` selects a font already installed under
`/mnt/sdcard/frogui/fonts/`. `[frogui] background_image` loads an image from
`/mnt/sdcard/frogui/schemes/<selected-scheme>/` (or the active style folder when
`Default` is selected). A picked user wallpaper still takes precedence. Paths
are limited to a filename to prevent escaping the theme folder.

The FrogUI extension `[frogui] home_layout=switch` enables a theme-provided
launcher layout. Its selected-state canvases live under
`schemes/<name>/640x480/image/static/muxlaunch/` or
`schemes/<name>/640x480/image/wall/muxlaunch/`. Full-screen images are used
directly when a theme supplies them. The optional default wall canvas lives at
`schemes/<name>/640x480/image/wall/default.png`, and battery glyphs at
`schemes/<name>/glyph/header/`. The canvas is aspect-fitted for other panel
sizes. Screen-specific INIs under `schemes/<name>/640x480/scheme/` are loaded
through the same renderer (`muxlaunch` for Home, `muxplore` for Games and
Recents, and `default` for Apps and Settings). Each view starts from the
selected profile before applying its own INI, so settings do not leak between
screens.

This is a renderer compatibility layer, not a `.muxthm` archive installer.
Screen background and header/list/footer colors honor MuOS alpha zero as
transparent/inherited; partial opacity, LVGL-only animation, MuOS `.bin`
fonts, and unrelated MuOS screens remain unsupported.
`ArtBookNext.ini` ports the compatible palette and transparent selection from
Art Book Next. Its carousel maps the supported actions to Explore Content,
Favourites, History, Apps, and Configuration. The archive's `default.ini` is
used for FrogUI's Games/Recents browser profile (`muxplore.ini`); system
folders are read from `/mnt/sdcard/roms/` after opening Explore Content. The
theme's separate Collection, Info, and Reboot actions do not have matching
FrogUI screens yet; shutdown is available from Apps. MuOS's top-level tabs are
not drawn for imported schemes. The MuOS font is not compatible; FrogUI keeps
the active FrogUI font.

Analogue, Def-not-SNES, dsOS, GbOS, MinUIfied Grid, MinUIfied Vertical, PipBoy,
PipBoy Frameless, and PipBoy Tube are adapted from their 640×480 MuOS profiles.
Their launcher art and colors are used where FrogUI has a matching screen; MuOS `.bin` fonts, sound,
boot logos, overlays, and other MuOS-only pages are not loaded.

Optional `style-profiles/<name>.json` files set per-theme defaults for a
renderer scheme. A missing scheme profile falls back to the selected built-in
style profile; saved user overrides still take priority unless
`ignoreOverrides` is true.
