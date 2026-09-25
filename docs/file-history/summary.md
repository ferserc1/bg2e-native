# File History System — Implementation Plan

## Problem statement

Currently, every file-picking control in `bg2e::ui` (e.g. `TextureWidgets::selectTexture()`,
used by `MaterialEditor` for albedo/normal/metallic/roughness/AO/light-emission textures)
opens the native file dialog directly on every click. There is no memory of previously
selected files, so re-picking an image used moments ago requires navigating the file
system again. Additionally, file-type filter knowledge (which extensions belong to
"images", "3D models", etc.) is scattered across UI code (`FileDialog::imageFilters`
is referenced directly from widgets).

Goals:

1. A generic, per-type file history in `bg2e::app` (no persistence, per-run, with `clear()`).
2. File **types** registered by name with a list of extensions (broader than a single
   extension: e.g. `image` = all supported image formats, `bg2model` = `.bg2`,
   `model3d` = `.bg2,.obj,.gltf,.glb`). Known types are auto-registered.
3. A reusable `bg2e::ui` history-picker widget: if the history is empty it opens the
   native file dialog directly; otherwise it shows an ImGui popup with a fixed
   "Open file..." button (outside the scroll area) and a scrollable list of previously
   selected files (thumbnail + file name; name only for non-image types).
4. UI widgets never call `FileDialog` directly anymore — they go through the history
   widget, which also encapsulates adding selections to the history.
5. Integration into `TextureWidgets` so the material editor texture pickers gain the
   history behavior transparently.

## Proposed solution

Three new components plus integration:

- **`bg2e::app::FileHistory`** — singleton. Owns the type registry (name →
  `FileDialog::FileFilters`) and one MRU list of paths per type. The known types
  (`image`, `bg2model`, `model3d`) are registered automatically in the constructor,
  so the first `FileHistory::get()` performs full initialization. No persistence;
  `clear()` / `clearAll()` reset state.
- **`bg2e::ui::FileHistoryWidget`** — ImGui picker bound to a history type. Draws a
  trigger button (text or image); on click, delegates to `openPicker()`: empty history
  → `FileDialog::getOpenFilePath()` with the filters from `FileHistory`; non-empty →
  `ImGui` popup with fixed "Open file..." button + scrollable entry list. Any chosen
  file is committed via `FileHistory::add()` internally. Optional thumbnail provider
  callback for image previews (name-only rows when unset or on load failure).
  **Its header does not expose `imgui.h`**: textures cross the API as an opaque
  64-bit `TextureID` (compatible with `ImTextureID` and `VkDescriptorSet`).
- **`TextureWidgets` integration** — `selectTexture()` uses an internal
  `FileHistoryWidget` (type `image`) instead of calling `FileDialog` directly.
  Thumbnails are resolved through `utils::TextureCache`; the required
  `render::Engine*` is supplied via `TextureWidgets::init(render::Engine*)` (stack
  object → `init` convention). `MaterialEditor` gets an `init(render::Engine*)` that
  forwards the engine to its six `TextureWidgets` members.

### Architecture diagram

```
+----------------------------- bg2e::app -----------------------------+
|  FileHistory (singleton)                                            |
|  -----------------------------------------------------------------  |
|  type registry:  "image"    -> { "Images",  "jpg,jpeg,png,bmp,webp" }|
|                  "bg2model" -> { "bg2e model", "bg2"                }|
|                  "model3d"  -> { "3D models",  "bg2,obj,gltf,glb"   }|
|  histories:      type -> [path(MRU first), ...]                     |
|  API: get/registerType/filtersFor/add/entries/empty/clear/clearAll  |
+--------------------------^------------------------------------------+
                           | uses (allowed: ui -> app)
+--------------------------|------------------------------------------+
|                   bg2e::ui                                          |
|                                                                     |
|  FileHistoryWidget (historyType)                                    |
|   - draw() / drawImageButton()  -> trigger button                   |
|   - openPicker():                                                   |
|       history empty  -> FileDialog::getOpenFilePath(filtersFor(t))  |
|       otherwise      -> ImGui popup:                                |
|            [Open file...]   <- fixed, outside scroll                |
|            +----------------------+                                 |
|            | (thumb) file1.png    |  <- BeginChild, scrollable      |
|            | (thumb) file2.jpg    |                                 |
|            +----------------------+                                 |
|   - commitSelection(path) -> FileHistory::add(type, path)           |
|   - optional ThumbnailProvider (ImTextureID per path)               |
|                                                                     |
|  TextureWidgets                MaterialEditor                       |
|   - _filePicker: FileHistoryWidget("image")                         |
|   - init(Engine*) -> wires thumbnail provider (TextureCache)        |
|   - selectTexture() uses _filePicker (no direct FileDialog)         |
|   - MaterialEditor::init(Engine*) forwards to its 6 widgets         |
+---------------------------------------------------------------------+
```

### Dependency check

- `bg2e::app::FileHistory` depends only on `common.hpp`, `FileDialog.hpp`
  (`FileFilters`) and the STL — allowed for layer 7 (`app`).
- `bg2e::ui::FileHistoryWidget` depends on `app::FileHistory`, `app::FileDialog`
  and ImGui — allowed (UI already depends on `app`, see `TextureWidgets.cpp`).
- `TextureWidgets` additionally uses `utils::TextureCache` (layer 6) — UI (layer 8)
  may depend on it.

## Files to create / modify

| File | Action | Description |
|------|--------|-------------|
| `lib/include/bg2e/app/FileHistory.hpp` | Create | Singleton: type registry + per-type MRU path history |
| `lib/src/bg2e/app/FileHistory.cpp` | Create | Implementation + auto-registration of known types |
| `lib/include/bg2e/app/all.hpp` | Modify | Add `FileHistory.hpp` include |
| `lib/include/bg2e/ui/FileHistoryWidget.hpp` | Create | ImGui history picker widget (popup + file dialog fallback) |
| `lib/src/bg2e/ui/FileHistoryWidget.cpp` | Create | Widget implementation |
| `lib/include/bg2e/ui/all.hpp` | Modify | Add `FileHistoryWidget.hpp` include |
| `lib/include/bg2e/ui/TextureWidgets.hpp` | Modify | Add `init(Engine*)`, `FileHistoryWidget` member, thumbnail cache |
| `lib/src/bg2e/ui/TextureWidgets.cpp` | Modify | `selectTexture()` uses history widget; thumbnail provider via `TextureCache` |
| `lib/include/bg2e/ui/MaterialEditor.hpp` | Modify | Add `init(render::Engine*)` |
| `lib/src/bg2e/ui/MaterialEditor.cpp` | Modify | `init()` forwards engine to the six `TextureWidgets` |
| `apps/model_edit/src/SubmeshWindow.cpp` | Modify | Call `_materialEditor.init(engine)` at setup |
| `apps/bg2e_composer/src/SubmeshWindow.cpp` | Modify | Call `_materialEditor.init(engine)` at setup |

## Steps

1. [step-01-file-history-singleton.md](step-01-file-history-singleton.md) — `bg2e::app::FileHistory` singleton
2. [step-02-file-history-widget.md](step-02-file-history-widget.md) — `bg2e::ui::FileHistoryWidget`
3. [step-03-texture-widgets-integration.md](step-03-texture-widgets-integration.md) — `TextureWidgets` + `MaterialEditor` integration
4. [step-04-app-wiring.md](step-04-app-wiring.md) — wiring in `model_edit` and `bg2e_composer`

## Notes

- **No `imgui.h` in engine headers**: engine public headers must never include
  `imgui.h` or expose ImGui types (`ImTextureID`, `ImVec2`, ...). ImGui is confined
  to `.cpp` files and existing `bg2e::ui` wrappers (`Button`, `Text`, ...). Where a
  texture identifier must cross a public API, use an opaque 64-bit type
  (`FileHistoryWidget::TextureID = uint64_t`, compatible with `ImTextureID` and
  `VkDescriptorSet`) and cast at the `.cpp` boundary.
- **Thread safety**: all components are UI-thread only (ImGui is single-threaded, the
  file dialog is synchronous and modal). No locking is added, consistent with the rest
  of `bg2e::ui`/`bg2e::app`.
- **Vulkan resource safety**: picking a file happens mid-frame while the old texture
  may still be in flight. The existing deferred-texture mechanism
  (`setDeferredTexture()` → swap on next `drawImage`/`imageButton`) is preserved.
  Thumbnail descriptor sets are removed in `cleanup()` after
  `device().waitIdle()`, following the existing `clearDS()` pattern.
- **No persistence** for now: history lives for the current run only. The API
  (`add`/`entries`/`clear`) is designed so a persistence backend can be added later
  without changing callers.
- **Graceful degradation**: if `TextureWidgets::init()` was never called, or a
  thumbnail fails to load (deleted/moved file), the history rows show file names only.
- **Build**: no CMake changes needed — the build glob-autodetects new files under
  `lib/include`/`lib/src`.
