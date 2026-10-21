# DrawTextureWidgets

**Header:** `<bg2e/ui/DrawTextureWidgets.hpp>`  
**Namespace:** `bg2e::ui`

`DrawTextureWidgets` draws a texture selection/clear row for a shared
`draw::Texture`. It does not mutate the material or upload during UI drawing.
`draw(UserInterface&, label, texture)` returns Result with selected, cleared and
path fields. Apply intent at the application's stopped scene/GPU boundary.

The widget tracks preview state in the UI session. Call `cleanup()` before
tearing down the UI/engine session so preview resources are released. File
history displays names; it does not load thumbnail files during UI drawing.
See [`DrawMaterialEditor`](Material_Editors.md#drawmaterialeditor) and the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md).
