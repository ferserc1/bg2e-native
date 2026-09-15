# ResourcePicker

**Header:** `<bg2e/ui/ResourcePicker.hpp>`  
**Namespace:** `bg2e::ui`

A stateless file-backed resource field used by `ReflectionWidget`.

```cpp
static bool ResourcePicker::draw(
    const std::string& label,
    std::filesystem::path& value,
    const reflection::PropertyMetadata& metadata,
    bool readOnly = false);
```

The widget combines an editable path field and a same-line **Browse** button.
The native dialog uses `resourceKind` as its filter label and
`resourceExtensions` as its extension list. Leading dots and wildcards are
normalized; an empty list falls back to all files.

Canceling leaves the value unchanged. If `resourcePathIsProjectRelative` is
true, a selected file is made relative to the current project directory when
possible. `readOnly` disables both editing and browsing. The function returns
`true` only when the path changes.

Use [`PropertyBuilder::resource()`](../reflection/Builder.md#propertybuildert)
to supply this metadata.

The model editor wires `NodeEditor::onResourceChanged()` for Environment
components. It validates the selected image, loads it through the active render
engine's texture cache, refreshes the scene/environment, and marks the document
dirty. A failed load returns `false`, so `ComponentInspector` restores the
previous path. Supported Environment extensions are `hdr`, `exr`, `jpg`,
`jpeg`, `png`, `bmp`, and `webp`.
