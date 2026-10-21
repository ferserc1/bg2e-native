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

`PropertyMetadata::resourceCanClear` defaults to false. Enabling it adds Clear and allows an explicit empty path; readOnly also disables clearing. Browse cancellation always retains the current path.

Draw Composer's Environment image property opts into Clear and filters `.hdr`/`.exr`. Edits are queued in place; renderer synchronization realizes only the active source. Invalid HDR retains requested fields and last-good GPU resources with visible diagnostics. This differs from a resource-validation callback that rejects a path and restores the old value. See [Environment](../draw/Environment.md) for selection, color persistence and alias/clearing rules.
