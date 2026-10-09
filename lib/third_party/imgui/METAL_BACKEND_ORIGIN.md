# Metal backend provenance

- Upstream: https://github.com/ocornut/imgui
- Revision: `e3979c20986d06fb4039014c2c2b1e4490a27a95` (2025-11-13).
- Core version: Dear ImGui 1.92.5 WIP (`IMGUI_VERSION_NUM` 19245).
- Files originally copied: `backends/imgui_impl_metal.h` and
  `backends/imgui_impl_metal.mm`, flattened into this directory.
- The header remains unchanged; the implementation has the local compatibility
  adjustments listed below.
- License: upstream MIT license, reproduced in `LICENSE.txt`.

Revision identity was established from byte-identical local `imgui.h` and
`imgui_internal.h`, and matching implementation bodies in `imgui.cpp`,
`imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp` and `imgui_demo.cpp`.
Existing engine copyright banners replace upstream introductory comments in
these local implementation files; no core implementation was updated here.

The Metal backend is compiled only on Apple platforms. Its supported Metal-cpp
bindings are selected by `IMGUI_IMPL_METAL_CPP` consistently for the vendored
`.mm` and `ui/ImGuiMetalBackend.cpp`. The `.mm` is compiled with ARC so its
texture ownership bridges and Objective-C strong properties manage lifetimes
correctly.
The upstream translation unit owns the Objective-C bridge; no new Metal-cpp
implementation macros or duplicate framework implementation units are added.
The UI adapter includes Metal-cpp declarations only inside its macOS guard.

## Local compatibility adjustments

- Guard the two legacy `autorelease`/`release` branches with
  `!__has_feature(objc_arc)` while retaining the official Metal-cpp overloads.
- Initialize the strong Metal context member with `nil` instead of zeroing its
  owning C++ object with `memset`.
- Select Shared texture storage on Apple Silicon. Preserve Managed storage on
  Intel macOS/Mac Catalyst for the legacy discrete GPU path. The SDK 27 enum
  deprecation is suppressed only around that intentional Intel assignment;
  no general warning suppression is applied.

These patches do not change frame submission, presentation or ImGui APIs.
