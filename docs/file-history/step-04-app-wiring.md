# Step 04 — App wiring (`model_edit` + `bg2e_composer`)

## Files

- Modify `apps/model_edit/src/SubmeshWindow.cpp`
- Modify `apps/bg2e_composer/src/SubmeshWindow.cpp`

## Design

Both apps host a `bg2e::ui::MaterialEditor _materialEditor;` member inside their
`SubmeshWindow` (stack object). `SubmeshWindow::init(AppDelegate*)` already receives
the delegate, which exposes the engine through the public
`RenderLoopDelegate::engine()` accessor (RenderLoopDelegate.hpp:60). One line per
app wires the engine into the material editor so texture pickers get history
thumbnails.

## `apps/model_edit/src/SubmeshWindow.cpp`

In `SubmeshWindow::init(AppDelegate * delegate)` (SubmeshWindow.cpp:22), add after
`_materialEditor.setSelectionManager(...)` (line 39):

```cpp
    _materialEditor.setSelectionManager(delegate->selectionManager());
    _materialEditor.init(delegate->engine());   // add
```

## `apps/bg2e_composer/src/SubmeshWindow.cpp`

Same change in its `SubmeshWindow::init(AppDelegate * delegate)` (line 22), after
`_materialEditor.setSelectionManager(delegate->selectionManager());` (line 39):

```cpp
    _materialEditor.setSelectionManager(delegate->selectionManager());
    _materialEditor.init(delegate->engine());   // add
```

(No header changes needed in either app.)

## Verification

There is no test framework; the examples/apps are the verification surface:

1. Build:
   ```sh
   cmake --build build
   ```
2. Run `bin/linux/model_edit` (or `bg2e_composer`):
   - Open a model, select a submesh so the material editor appears.
   - **Empty history**: click a texture slot button (e.g. albedo) → the native file
     dialog opens directly. Pick an image → it applies and appears in the slot.
   - **Non-empty history**: click the same (or a different, e.g. normal) texture
     slot → a popup appears with a fixed **"Open file..."** button at top and the
     previously picked image listed below (thumbnail + file name, full path in
     tooltip). Clicking the entry applies it.
   - **Shared per type**: an image picked for albedo also appears in the normal /
     metallic / roughness / AO / light-emission pickers (all use type `image`).
   - **Open file from popup**: "Open file..." opens the native dialog; the new file
     is applied and moved to the top of the history.
   - **Popup dismissal**: clicking outside the popup closes it without changes.
   - **Graceful degradation**: temporarily comment out the `init()` call → pickers
     still work; history rows show file names without thumbnails.
3. Check for Vulkan validation errors on exit (descriptor set cleanup path:
   `TextureWidgets::cleanup()` → `clearThumbnails()` after `waitIdle()`).

## Notes

- `delegate->engine()` is valid at `SubmeshWindow::init()` time because
  `AppDelegate::init(engine)` runs first (it creates the workspace/windows after
  `DefaultRenderLoopDelegate::init(engine)` stores `_engine`).
- Other apps/examples using `TextureWidgets` or `MaterialEditor` directly (search
  for `selectTexture` / `MaterialEditor` under `examples/` before merging) get
  working pickers without thumbnails until they call `init(engine)`.
