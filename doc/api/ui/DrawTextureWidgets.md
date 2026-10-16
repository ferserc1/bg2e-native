# DrawTextureWidgets

**Header:** `<bg2e/ui/DrawTextureWidgets.hpp>`  
**Namespace:** `bg2e::ui`

`DrawTextureWidgets` draws a texture selection/clear row for an experimental
`draw::Texture`. Unlike the production [`TextureWidgets`](TextureWidgets.md),
it does not mutate the bound material or upload a replacement while UI drawing
is in progress. `draw()` returns a `Result` containing a selected path or clear
intent; the application applies that intent at its safe scene/GPU boundary.

The widget tracks preview state in the UI session. Call `cleanup()` before
tearing down the UI/engine session so preview resources are released. File
history displays names; it does not load thumbnail files during UI drawing.
See [`DrawMaterialEditor`](Material_Editors.md#drawmaterialeditor) and the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md).
