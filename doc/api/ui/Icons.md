# PNG icons, Icon and IconManager

**Headers:** `<bg2e/ui/Icon.hpp>`, `<bg2e/ui/IconManager.hpp>`, `<bg2e/ui/IconDrawOptions.hpp>`  
**Namespace:** `bg2e::ui` · also included by `<bg2e/ui/all.hpp>`

Pass a `std::filesystem::path` to UI widgets. They load a PNG on first use and
share its cached resource automatically. Draw inside a normal UI window/frame
on the owning UI thread. These fragments assume application handlers such as
`saveDocument()` are defined by the host.

```cpp
#include <bg2e/ui/Image.hpp>
#include <bg2e/ui/Button.hpp>
#include <filesystem>

const std::filesystem::path savePng = "assets/icons/save.png";
bg2e::ui::Image::icon(savePng, 24.0f);
if (bg2e::ui::Button::iconButton("save-document", savePng, 20.0f,
    {.tooltip = "Save document"}))
{
    saveDocument();
}
if (bg2e::ui::Button::iconTextButton("save-copy", "Save copy", savePng, 20.0f))
{
    saveCopy();
}
```

All three controls share one icon resource in this session despite their
different IDs and sizes. The path identifies the image; an explicit button ID
identifies the control. Never derive button identity solely from a PNG path.
See [Image](Image.md), [Button](Button.md), [Menu](Menu.md#png-icons) and
[Toolbar](Toolbar_and_StatusBar.md#toolbar-icons-and-configuration) for widget APIs.

Toolbars default to IconsAndText, so supplying iconPath displays the image beside
its label. An explicit Text setting hides toolbar icons even when paths exist;
select Icons and text in UI Settings to display them. This preference does not
change standalone icon buttons or menu icons.

## Explicit resources

Ordinary widget calls do not require a manager reference. For retained handles,
`UserInterface::icons()` exposes the same manager:

```cpp
// During delegate init(engine, ui), or later on the UI thread:
std::shared_ptr<bg2e::ui::Icon> saveIcon =
    ui->icons().load(std::filesystem::path{"assets/icons/save.png"});
```

Store that handle in the delegate. In a subsequent draw callback:

```cpp
bg2e::ui::Image::icon(saveIcon, 24.0f);
if (bg2e::ui::Button::iconButton("save-document", saveIcon, 20.0f))
{
    saveDocument();
}
```

`load()` is synchronous demand loading. It reads/decodes and uploads on a cache
miss, so the first use can pause the UI. There is no preload operation, background
loader, automatic file scan or image atlas. IconManager is created/owned by
UserInterface, cannot be copied and must not outlive or be deleted independently
of its UI. `icons()` is available during delegate initialization, before the
session's `initialized()` flag is set; it raises `std::logic_error` before
manager creation or after cleanup.

```cpp
// IconManager public resource operation:
std::shared_ptr<Icon> load(const std::filesystem::path& path);

// Icon public queries:
bool valid() const;
bool isFallback() const;
const std::filesystem::path& source() const;
```

Icon objects are opaque, noncopyable resources; share their `shared_ptr` handles.
They expose no ImGui or native texture identifiers.

| Query | Meaning |
| --- | --- |
| `source()` | Source identity retained by the resource; normally an absolute lexically normalized path. |
| `isFallback()` | Source resolution/decoding failed. This flag remains meaningful after cleanup. |
| `valid()` | The owning session is live and retains a drawable image, including a fallback image. It does not establish that the source loaded successfully or that this is the current UI context. |

A live fallback with no GPU image has `isFallback() == true` and
`valid() == false`; widgets still render a text question mark. Do not hide every
icon whose `valid()` query is false if you want that substitute to remain visible.

## Source and cache policy

- Keys are native `std::filesystem::path` values, made absolute and lexically
  normalized. Relative paths resolve against the current working directory on
  each call; no automatic asset-directory prefix is added. Prefer an application
  asset-root path when the working directory may change.
- Paths are not lowercased or canonicalized. Symlink aliases and case variants
  are not guaranteed to share a resource. Size, tint and widget identity never
  participate in source keys.
- If absolute resolution fails, its supplied lexical path serves as the failure
  key. A later successful absolute resolution can produce a distinct key.
- One strong cache retains successful icons and failed-source records until UI
  cleanup, normally the application lifetime. Releasing application handles or
  changing a widget path does not evict earlier resources.
- Cache hits perform no PNG read/decoding or GPU upload. Relative-key resolution
  still queries the working directory. GPU registration is lazy on first drawing
  and reused while the backend generation remains unchanged.
- Empty paths return a null handle and mean no icon. A nonempty missing, corrupt
  or unsupported source yields a cached failure and logs once per key. The loader
  accepts PNG extensions through the existing case-insensitive format helper.
- A corrected file at the same key is not automatically retried. There is no
  cache-clear/eviction, reload or file-watching API; a new UI session reads it anew.

The loader reuses `db::loadImage` for RGBA8 decoding. That existing loader uses
narrow file-path strings internally; some Windows Unicode paths can therefore
fail and display the substitute even though cache identities retain native paths.

## Missing sources and lifecycle errors

Failed sources retain their own source identity and share a procedural question
mark texture. Its upload is attempted once per session; if that best-effort upload
fails, widgets draw `?` as text in the same icon box. Failure records prevent disk
access and repeated diagnostics on each frame. Normal icon GPU upload/registration
errors, allocation exhaustion and invalid session use are not silently converted
into missing assets; they can propagate exceptions.

Explicit `load()` requires the owning UI context to be active. Using it from a
wrong context raises `std::logic_error`. Path-based image/button wrappers without
an active session return without loading/drawing; buttons return false. Resource
image/button wrappers similarly reject stale or cross-session handles. Menu
wrappers can still draw native text/gutters in a bare ImGui context but cannot
resolve icons without UserInterface; use them within the normal UI lifecycle.

UI cleanup invalidates handles before destroying the backend. A retained handle
cannot attach itself to a later session, even if native context addresses are
reused. Reacquire resources from the new UserInterface. Release application
handles when no longer needed and clean up UI before engine/device destruction.

## Size, transparency and color

PNG source dimensions are independent of display size. Image/button `size` is
a positive finite logical square, default 20; it is multiplied by
`UserInterface::getScale()` once. The source is centered and fitted proportionally
inside it. A 48×24 PNG drawn with size 24 occupies 24×12 logical units in a 24×24
box. Fonts and style padding are already scaled. Toolbar defaults use size 16;
menus use the already-scaled current text-line height.

Use straight-alpha RGBA PNGs, including transparent backgrounds. The upload
preserves authored RGBA bytes in RGBA8_UNORM and uses the current ImGui numeric
color treatment: no additional sRGB decoding, material gamma transform, PNG
color-profile processing or premultiplication is introduced. Sampling is linear,
with clamp-to-edge and no mipmaps. Supply enough source pixels for the largest
expected display scale.

`IconDrawOptions::tint` multiplies the sampled color; white preserves the source.
A white monochrome PNG can be tinted using `base::Color`. Multicolor images will
also be multiplied. Style alpha and disabled alpha apply to image/text drawing;
button text uses the current UI text color rather than the image tint.

## Shared infrastructure with TexturePreview

[TexturePreview](TexturePreview.md) and icons share private image registration,
exact image/sampler ownership, backend generation/context checks and drawing.
There is no shared source-path cache with previews or general scene textures.

Icons retain their entries strongly for the whole session. A preview retains a
snapshot of an existing draw texture; orphaned snapshots are retired after GPU
completion. Cleanup invalidates both resource kinds before backend destruction.
TexturePreview's public API, unscaled integer dimensions and snapshot behavior
remain unchanged.
