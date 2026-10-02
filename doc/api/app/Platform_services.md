# Platform Services

**Headers:** `<bg2e/app/FileDialog.hpp>`, `<bg2e/app/FileHistory.hpp>`,
`<bg2e/app/MessageBox.hpp>`, `<bg2e/app/GPUSelectionDialog.hpp>`  
**Namespace:** `bg2e::app`

## FileDialog

`FileDialog` exposes native open, save, and folder-selection dialogs. Filters
map a visible label to an extension expression.

```cpp
FileDialog::FileFilters filters {
    { "glTF scene", "gltf,glb" }
};

auto input = FileDialog::getOpenFilePath(filters);
auto output = FileDialog::getSaveFilePath(filters);
auto folder = FileDialog::getPickFolderPath();
```

Empty paths mean that the user cancelled. Instance methods use filters supplied
through `setFilters()`. `imageFilters` contains the built-in image filter set.

## FileHistory

`FileHistory::get()` returns the singleton recent-file registry. Histories are
partitioned by registered type and contain at most 32 paths, most recent first.
Built-in types include `Image`, `Bg2Model`, and `Model3D`.

```cpp
auto& history = FileHistory::get();
history.registerType("Scenes", "glTF scene", { "gltf", "glb" });
history.add("Scenes", selectedPath);

for (const auto& path : history.entries("Scenes")) {
    // Build a recent-files action.
}
```

## MessageBox

`MessageBox` provides native information, warning, and error messages. Simple
overloads show the platform default confirmation button. Custom buttons carry
an application code, label, and optional Escape/Return default key.

```cpp
int result = MessageBox::showWarning(
    "Unsaved changes",
    "Close without saving?",
    {
        { 0, "Cancel", MessageBox::Esc },
        { 1, "Close", MessageBox::Return }
    }
);
```

## GPUSelectionDialog

`GPUSelectionDialog(appId).run()` presents the available Vulkan devices and
returns the selected physical-device properties, or an empty pointer when no
selection is made.

