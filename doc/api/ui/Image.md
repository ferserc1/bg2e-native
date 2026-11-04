# Image and IconDrawOptions

**Headers:** `<bg2e/ui/Image.hpp>`, `<bg2e/ui/IconDrawOptions.hpp>`  
**Namespace:** `bg2e::ui`

Draw a noninteractive PNG icon directly from a path:

```cpp
const std::filesystem::path infoPng = "assets/icons/info.png";
bg2e::ui::Image::icon(infoPng, 24.0f, {
    .tint = bg2e::base::Color::White(),
    .tooltip = "Document information",
    .sameLine = true,
    .disabled = false
});
```

Call during UI drawing inside a window. The path resolves through the current
session cache; no explicit resource setup is needed. See [PNG icons](Icons.md)
for loading, failure, source normalization and lifetime contracts.

## Signatures

```cpp
// Members of ui::Image:
static void icon(const std::filesystem::path& path,
    float size = 20.0f, const IconDrawOptions& options = {});
static void icon(const std::shared_ptr<Icon>& resource,
    float size = 20.0f, const IconDrawOptions& options = {});

struct IconDrawOptions {
    base::Color tint = base::Color::White();
    std::string tooltip;
    bool sameLine = false;
    bool disabled = false;
};
```

The signatures above describe static class members; include the headers rather
than redeclaring them. `size` defines a square in logical UI units, scaled once
by the interface factor. Rendering preserves aspect ratio and centers the image
inside the reserved square. The widget creates one noninteractive layout item,
with no button background or click result. It is also the tooltip hover target.

| Option | Behavior |
| --- | --- |
| `tint` | Multiplies image channels, including alpha; default white preserves PNG colors. |
| `tooltip` | Optional hover text, also available for a disabled item. |
| `sameLine` | Places the item after the preceding item using normal UI spacing. |
| `disabled` | Applies normal disabled/style alpha; the item remains noninteractive. |

An empty path or null resource draws nothing. A nonempty failed path reserves
the same square and displays the shared image or text question mark. No active
session or a stale/cross-session resource causes a no-op. Invalid size raises
`std::invalid_argument` when a live icon is actually drawn. Use a typed empty
path or typed null resource if selecting an overload without an icon.

To render actions, use [iconButton/iconTextButton](Button.md#icon-buttons), which
share the resource and size policy but create a single navigable button.
