# MuOS-inspired theming plan

## Goal

Add a data-driven display theme layer inspired by MuOS scheme files, while
keeping FrogUI's existing RGB565 renderer, input, navigation, and runtime. LVGL
is not part of this change: it would replace the widget and display pipeline,
not provide a drop-in theme loader.

## Shape

1. Keep palette selection and style selection separate. The existing palette
   remains the fallback for each colour; a style scheme may override specific
   colours without affecting other palettes.
2. Add one renderer theme model containing display geometry and optional colour
   tokens. Renderer functions read from this model instead of embedding their
   layout constants at each call site.
3. Load built-in baseline values for every style, then apply an optional
   `/mnt/sdcard/frogui/schemes/<style>.ini` override. Missing files retain the
   built-in appearance; invalid values are ignored. Restrict style names before
   forming paths.
4. Convert the current Vertical geometry into the built-in `vertical` scheme.
   Preserve the existing palette, text sizing, and interaction behavior.
5. Keep system behavior profiles (`style-profiles/*.json`) separate; they
   continue to control settings such as quick resume and folder-style settings.

## Scheme format (first slice)

`[renderer]` supports `header_height`, `row_height`, `horizontal_padding`,
`content_top`, `legend_height`, `selected_radius`, `decorative_radius`,
`thumbnail_width`, and `thumbnail_height`. `[colours]` supports `background`,
`text`, `selection`, `selection_text`, `header`, `folder`, `legend`,
`legend_background`, and `disabled`, as six-digit RGB values. Omitted colours
inherit the currently selected palette.

## Delivery sequence

1. Add the renderer theme model and bounded INI reader.
2. Move the renderer's shared geometry defaults into that model.
3. Load the selected style's model alongside its existing behavior profile.
4. Build FrogUI and run its existing QEMU/device smoke path; compare Vertical
   rendering at 640x480 and 854x480 against the previous constants.
5. Integrate the FrogUI commit into the `r36sx` branch, update TreeFrogUI's
   submodule pin, and package the example scheme for SD deployments.

The first implementation intentionally does not recreate LVGL widgets,
animations, or MuOS-specific theme assets. Those require explicit mappings to
FrogUI drawing functions and can be added as the scheme format grows.
