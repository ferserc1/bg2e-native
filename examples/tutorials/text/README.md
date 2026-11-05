# Editable Hello World text

Target: **`tutorial_text`**. Complete source: [src/main.cpp](src/main.cpp).
Follow the [beginner walkthrough](../../../doc/tutorials/text/README.md) for
prerequisites, platform-specific configuration/build/run commands and every
scene/lifecycle step. Uses the production draw renderer and existing UI wrappers.

The application opens a 1100x760 window with a fixed camera, a neutral procedural
floor, a raised blue `Hello World` label and one directional light. On devices
supporting the renderer's compute ray-query shadows, the floor receives the actual
glyph silhouette, not the text rectangle. The small **Edit the text** window
changes content live; blank content removes glyphs and their shadows. There is no
camera navigation, Composer/history subsystem, unrelated model, HDR input or test
mode. **Reload font / retry text** explicitly retries corrected source/resource
failures without per-frame filesystem/diagnostic loops.

Missing fonts leave text transparent while the floor/UI remain usable. Query
shadow availability depends on backend/device/driver capabilities; unsupported
systems can display the scene without promising a shadow. See guide troubleshooting.

`bundle_app_sdl` deploys engine shaders, runtime dependencies and the normal asset
tree. The only external authored resource this scene needs is the canonical
`assets/fonts/didact-gothic/DidactGothic-Regular.ttf`, with adjacent
[OFL.txt](../../../assets/fonts/didact-gothic/OFL.txt) and
[pinned provenance/checksum](../../../assets/fonts/didact-gothic/README.md).
The rasterizer's separate [MIT notice](../../../assets/licenses/stb_truetype-MIT.txt)
is also in the bundled assets. Preserve these notices when redistributing.
The root-level legacy font is not used, and no system font/download is required.

Native compilation and visual acceptance are user-run activities. Source/diff
review is not evidence of a successful build or backend execution.
