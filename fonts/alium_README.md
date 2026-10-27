# Allium palette ports and font

Select an `alium_*` palette under Appearance → Theme and `alium_Nunito`
under Appearance → Font. Turn Background Images off for the plain palette.
The font is **Nunito Bold**, the exact, unmodified `Nunito.ttf` bundled with
Allium, renamed on disk to match the requested prefix. Its internal font name
remains Nunito Bold. It is licensed under SIL OFL 1.1; see `alium_Nunito-OFL.txt`.

Sources:

- [Allium default font configuration](https://github.com/goweiwen/Allium/blob/ee0dda979bc02649fdda48b277eaa60938273465/crates/common/src/stylesheet.rs)
- [Bundled Nunito font](https://github.com/goweiwen/Allium/blob/ee0dda979bc02649fdda48b277eaa60938273465/static/.allium/fonts/Nunito.ttf)
- [Allium-Themes catalog](https://github.com/goweiwen/Allium-Themes/tree/05ed164e5e18628aa1e17acc1896a6c443045997/Themes)

The 12 palettes are `alium_default` (Allium), `alium_alliumboy`, `alium_blue`,
`alium_boxart`, `alium_brown`, `alium_bubblegum`, `alium_eva_01`, `alium_min`,
`alium_pastel`, `alium_snes`, `alium_spruceos`, and `alium_synthwave`.

These are color ports, not layout or wallpaper ports. Source RGB components
are converted to FrogUI's RGB565 format; alpha is omitted because the palette
has no opacity fields. Wallpaper-only backgrounds therefore become solid
colors. Theme selection and font selection remain independent; some other
Allium themes choose fonts other than Nunito.

FrogUI background, text, selection, selected text, and disabled colors map
directly to the matching Allium UI fields. Headers use `tab_selected_color`,
folders use `text_color`, legends use `button_hints.text_color`, and legend
bars use the UI background. Per-button colors have no FrogUI palette field.

## Allium-Themes license

MIT License

Copyright (c) 2025 Goh Wei Wen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
