# Step 03 — `TextureWidgets` + `MaterialEditor` integration

## Files

- Modify `lib/include/bg2e/ui/TextureWidgets.hpp`
- Modify `lib/src/bg2e/ui/TextureWidgets.cpp`
- Modify `lib/include/bg2e/ui/MaterialEditor.hpp`
- Modify `lib/src/bg2e/ui/MaterialEditor.cpp`

## Design

`TextureWidgets::selectTexture()` currently calls
`app::FileDialog::getOpenFilePath(app::FileDialog::imageFilters)` directly
(TextureWidgets.cpp:75). It is reworked to delegate file picking to an internal
`FileHistoryWidget` of type `FileHistory::Image`.

Because history rows should show image thumbnails, the widget needs GPU resources
(`render::Texture` + `VkDescriptorSet`). Following the engine convention — stack
objects receive the engine via an `init`-style method — `TextureWidgets` gets
`init(render::Engine*)`, which:

1. Stores the engine pointer.
2. Installs a `ThumbnailProvider` on the file picker that loads textures through
   `utils::TextureCache::get()` and registers ImGui descriptor sets.

If `init()` was never called, no provider is installed and the picker shows
file names only (graceful degradation).

`MaterialEditor` (also stack-resident in apps) gets `init(render::Engine*)` that
forwards the engine to its six `TextureWidgets` members.

> **Header rule**: no engine header may include `imgui.h` or expose ImGui types.
> All texture identifiers crossing these APIs use the opaque
> `FileHistoryWidget::TextureID` (`uint64_t`); `ImTextureID` /
> `ImGui_ImplVulkan_*` calls stay confined to `.cpp` files.

## `TextureWidgets.hpp` changes

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/ui/FileHistoryWidget.hpp>          // add

#include <memory>
#include <functional>
#include <unordered_map>                          // add

namespace bg2e {
namespace render { class Engine; }                // fwd decl (already pulled in by Texture.hpp, but explicit)

namespace ui {

class BG2E_API TextureWidgets {
public:

    TextureWidgets();                             // add: constructs _filePicker with FileHistory::Image

    // Stack-object convention: supply the engine after construction.
    // Enables image thumbnails in the file history picker.
    void init(render::Engine* engine);            // add

    // ... setEditTexture / setDeferredTexture / clearTexture / drawImage /
    //     imageButton / cleanup: unchanged ...

    bool selectTexture(const std::string& label,
                       std::function<std::shared_ptr<render::Texture>(base::Texture* tex)>);

    void cleanup();

protected:
    std::shared_ptr<render::Texture> _texture;
    std::shared_ptr<render::Texture> _deferredTexture;

    VkDescriptorSet _textureDS = VK_NULL_HANDLE;

    render::Engine* _engine = nullptr;            // add
    FileHistoryWidget _filePicker;                // add (type: FileHistory::Image)

    // path -> ImGui descriptor set for history thumbnails
    std::unordered_map<std::string, VkDescriptorSet> _thumbnailDS;   // add

    // Returns FileHistoryWidget::TextureID (opaque uint64_t) so this header
    // never exposes imgui.h / ImTextureID.
    FileHistoryWidget::TextureID thumbnailForPath(const std::filesystem::path& path); // add
    void clearThumbnails();                                                           // add

    void initDS();
    void clearDS();
    void updateDeferredTexture();
};

}
}
```

Constructor must initialize `_filePicker` with the image type:

```cpp
TextureWidgets::TextureWidgets()
    : _filePicker{ app::FileHistory::Image }
{
}
```

(Alternatively `_filePicker{ app::FileHistory::Image }` as an in-class initializer,
but `app::FileHistory` constants are only declared in the header — initialize in
the .cpp constructor to avoid ODR/linkage issues with `static const std::string`.)

## `TextureWidgets.cpp` changes

### `init()`

```cpp
void TextureWidgets::init(render::Engine* engine)
{
    _engine = engine;
    _filePicker.setThumbnailProvider(
        [this](const std::filesystem::path& path) -> FileHistoryWidget::TextureID {
            return thumbnailForPath(path);
        });
}
```

### Thumbnail provider

Uses `utils::TextureCache` (shared with the rest of the app, so history thumbnails
reuse already-loaded images) and caches one `VkDescriptorSet` per path. The ImGui
descriptor set is converted to the opaque `FileHistoryWidget::TextureID` at the
API boundary (both are 64-bit values):

```cpp
FileHistoryWidget::TextureID TextureWidgets::thumbnailForPath(const std::filesystem::path& path)
{
    if (_engine == nullptr)
    {
        return 0;
    }
    auto key = path.string();
    auto it = _thumbnailDS.find(key);
    if (it != _thumbnailDS.end())
    {
        return static_cast<FileHistoryWidget::TextureID>(it->second);
    }

    auto tex = utils::TextureCache::get().load(_engine, path);
    if (!tex.get())
    {
        return 0;   // missing/unloadable file: row shows name only
    }
    auto ds = ImGui_ImplVulkan_AddTexture(
        tex->sampler(), tex->image()->imageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    _thumbnailDS[key] = ds;
    return static_cast<FileHistoryWidget::TextureID>(ds);
}

void TextureWidgets::clearThumbnails()
{
    if (!_thumbnailDS.empty() && _engine != nullptr)
    {
        _engine->device().waitIdle();
        for (auto & [path, ds] : _thumbnailDS)
        {
            ImGui_ImplVulkan_RemoveTexture(ds);
        }
        _thumbnailDS.clear();
    }
}
```

Additional includes in the .cpp:

```cpp
#include <bg2e/app/FileHistory.hpp>
#include <bg2e/utils/TextureCache.hpp>
```

### `selectTexture()` rework

Replace the direct `FileDialog` call with the history picker. The existing
"Clear" button and label rendering stay as they are:

```cpp
bool TextureWidgets::selectTexture(const std::string& label,
    std::function<std::shared_ptr<render::Texture>(base::Texture* tex)> textureCallback)
{
    updateDeferredTexture();

    bool picked = _filePicker.drawImageButton(
        label.starts_with("##") ? "imagePick" + label : "imagePick##" + label,
        static_cast<FileHistoryWidget::TextureID>(_textureDS), 42, 42);

    if (picked)
    {
        const auto & filePath = _filePicker.selectedPath();
        base::Texture * texture = new base::Texture();
        texture->setImageFilePath(filePath.string());
        texture->setMagFilter(base::Texture::FilterLinear);
        texture->setMinFilter(base::Texture::FilterLinear);
        texture->setUseMipmaps(true);
        auto tex = textureCallback(texture);
        setDeferredTexture(tex);
    }

    if (Button::button(label.starts_with("##") ? "Clear" + label : "Clear##" + label, true)) {
        auto tex = textureCallback(nullptr);
        setDeferredTexture(tex);
    }

    if (!label.starts_with("##"))
    {
        Text::text(label, true);
    }

    return picked;
}
```

Notes:
- `drawImageButton()` is called **even when `_textureDS == VK_NULL_HANDLE`**
  (texture not yet set): the widget falls back to a text trigger button, so a slot
  with no texture can still be picked. This is a small behavior improvement over
  the current code, where `imageButton()` silently does nothing without a texture.
- The image-button ID scheme (`imagePick##<label>`) is preserved so ImGui state
  stays compatible.
- `app::FileDialog` include in this file is no longer needed by `selectTexture()`
  (remove `#include <bg2e/app/FileDialog.hpp>` if unused elsewhere).

### `cleanup()`

```cpp
void TextureWidgets::cleanup()
{
    clearDS();
    clearThumbnails();   // add
    _texture.reset();
    _deferredTexture.reset();
}
```

## `MaterialEditor.hpp` changes

```cpp
class BG2E_API MaterialEditor {
public:
    virtual ~MaterialEditor();

    // Stack-object convention: must be called once before draw() so texture
    // pickers can show history thumbnails. Safe to skip: pickers then show
    // file names only.
    void init(render::Engine* engine);   // add

    // ... rest unchanged ...

protected:
    render::Engine* _engine = nullptr;   // add (optional, kept for future use)
    // ...
};
```

`MaterialEditor.hpp` needs `namespace bg2e { namespace render { class Engine; } }`
forward declaration (it currently includes `render/MaterialBase.hpp`, which may
already provide it transitively — use an explicit fwd decl to be safe).

## `MaterialEditor.cpp` changes

```cpp
void MaterialEditor::init(render::Engine* engine)
{
    _engine = engine;
    _albedoWidget.init(engine);
    _normalWidget.init(engine);
    _metallicWidget.init(engine);
    _roughnessWidget.init(engine);
    _aoWidget.init(engine);
    _lightEmissionWidget.init(engine);
}
```

The `#include <bg2e/app/FileDialog.hpp>` at MaterialEditor.cpp:22 becomes unused —
remove it.

## Integration points

- Apps must call `MaterialEditor::init(engine)` once (step 04).
- Any other user of `TextureWidgets` outside `MaterialEditor` (search for
  `selectTexture` usages before merging) can optionally call `init(engine)`;
  without it the picker still works with name-only rows.
- Deferred texture swap (`setDeferredTexture`) is untouched: picked textures are
  applied on the next frame, so no in-flight descriptor set is invalidated by a
  mid-frame file pick.
- `TextureCache` keeps thumbnail sources alive for the app's lifetime; descriptor
  sets created here are removed in `cleanup()` after `waitIdle()` (same pattern as
  `clearDS()` at TextureWidgets.cpp:117).
