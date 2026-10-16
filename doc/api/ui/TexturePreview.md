# TexturePreview

**Header:** `<bg2e/ui/TexturePreview.hpp>`  
**Namespace:** `bg2e::ui`

`TexturePreview` is an opaque, session-scoped preview for a draw texture. It
exposes `valid()`, `drawImage(width, height)` and
`imageButton(id, width, height)` without exposing ImGui or native GPU types.
The preview is created and registered by the UI implementation; applications
normally use it through [`DrawTextureWidgets`](DrawTextureWidgets.md).

Preview registrations and resources are retained until submitted UI draws have
completed. A preview becomes invalid when its `UserInterface` session is
cleaned up, so do not retain or draw it across UI/engine teardown. See the
[UI overview](index.md) for the production and experimental draw widget split.
