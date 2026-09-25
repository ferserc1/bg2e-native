# Step 01 — `bg2e::app::FileHistory` singleton

## Files

- Create `lib/include/bg2e/app/FileHistory.hpp`
- Create `lib/src/bg2e/app/FileHistory.cpp`
- Modify `lib/include/bg2e/app/all.hpp` (add include)

## Design

A singleton (same pattern as `utils::TextureCache`) that owns:

1. **Type registry**: `typeName -> FileDialog::FileFilters`. Types are broader than a
   single extension; each type maps to a label + extension list in the exact
   `FileDialog::FileFilters` format (`{ label, "ext,ext,..." }`), so filters can be
   passed directly to `FileDialog::getOpenFilePath()`.
2. **Histories**: `typeName -> std::vector<std::filesystem::path>`, MRU-ordered
   (most recent first, duplicates moved to front), bounded by a max size.

Known types are registered **in the constructor**, so the first `FileHistory::get()`
initializes everything automatically:

| Constant | Name | Extensions |
|----------|------|-----------|
| `FileHistory::Image` | `"image"` | `jpg,jpeg,png,bmp,webp` (matches `FileDialog::imageFilters`) |
| `FileHistory::Bg2Model` | `"bg2model"` | `bg2` |
| `FileHistory::Model3D` | `"model3d"` | `bg2,obj,gltf,glb` |

No persistence — history lives only for the current run. `clear(type)` and
`clearAll()` reset state.

## Header (`lib/include/bg2e/app/FileHistory.hpp`)

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/FileDialog.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace bg2e {
namespace app {

class BG2E_API FileHistory {
public:
    static FileHistory& get();

    // Well-known type names (registered automatically in the constructor)
    static const std::string Image;     // "image"
    static const std::string Bg2Model;  // "bg2model"
    static const std::string Model3D;   // "model3d"

    // Type registry. extensions without dot: { "jpg", "png", ... }
    void registerType(const std::string& typeName,
                      const std::string& filterLabel,
                      const std::vector<std::string>& extensions);
    bool isTypeRegistered(const std::string& typeName) const;
    const FileDialog::FileFilters& filtersFor(const std::string& typeName) const;

    // History (MRU: most recent first). add() on an unregistered type
    // throws std::runtime_error.
    void add(const std::string& typeName, const std::filesystem::path& path);
    const std::vector<std::filesystem::path>& entries(const std::string& typeName) const;
    bool empty(const std::string& typeName) const;
    void clear(const std::string& typeName);
    void clearAll();

    static constexpr size_t MaxEntriesPerType = 32;

protected:
    FileHistory();           // registers known types
    virtual ~FileHistory() = default;

    static FileHistory * g_singleton;

    std::unordered_map<std::string, FileDialog::FileFilters> _typeFilters;
    std::unordered_map<std::string, std::vector<std::filesystem::path>> _histories;
};

}
}
```

Notes:
- Reuses `FileDialog::FileFilters` so the registry doubles as the single source of
  truth for file-type filters in the UI.
- `FileDialog.hpp` only needs `common.hpp` + `Preferences.hpp`; no heavy deps.

## Implementation (`lib/src/bg2e/app/FileHistory.cpp`)

```cpp
#include <bg2e/app/FileHistory.hpp>

#include <algorithm>
#include <stdexcept>

namespace bg2e::app {

const std::string FileHistory::Image    = "image";
const std::string FileHistory::Bg2Model = "bg2model";
const std::string FileHistory::Model3D  = "model3d";

FileHistory * FileHistory::g_singleton = nullptr;

FileHistory& FileHistory::get()
{
    if (g_singleton == nullptr)
    {
        g_singleton = new FileHistory();
    }
    return *g_singleton;
}

FileHistory::FileHistory()
{
    registerType(Image,    "Images",     { "jpg", "jpeg", "png", "bmp", "webp" });
    registerType(Bg2Model, "bg2e model", { "bg2" });
    registerType(Model3D,  "3D models",  { "bg2", "obj", "gltf", "glb" });
}

void FileHistory::registerType(const std::string& typeName,
                               const std::string& filterLabel,
                               const std::vector<std::string>& extensions)
{
    std::string extList;
    for (size_t i = 0; i < extensions.size(); ++i)
    {
        if (i > 0) extList += ",";
        extList += extensions[i];
    }
    _typeFilters[typeName] = FileDialog::FileFilters { { filterLabel, extList } };
    // Create empty history bucket so entries()/empty() are always safe
    _histories.try_emplace(typeName);
}

bool FileHistory::isTypeRegistered(const std::string& typeName) const
{
    return _typeFilters.find(typeName) != _typeFilters.end();
}

const FileDialog::FileFilters& FileHistory::filtersFor(const std::string& typeName) const
{
    auto it = _typeFilters.find(typeName);
    if (it == _typeFilters.end())
    {
        throw std::runtime_error("FileHistory::filtersFor(): unregistered type '" + typeName + "'");
    }
    return it->second;
}

void FileHistory::add(const std::string& typeName, const std::filesystem::path& path)
{
    if (!isTypeRegistered(typeName))
    {
        throw std::runtime_error("FileHistory::add(): unregistered type '" + typeName + "'");
    }
    auto & list = _histories[typeName];
    // MRU: remove existing occurrence, then push front
    list.erase(std::remove(list.begin(), list.end(), path), list.end());
    list.insert(list.begin(), path);
    if (list.size() > MaxEntriesPerType)
    {
        list.resize(MaxEntriesPerType);
    }
}

const std::vector<std::filesystem::path>& FileHistory::entries(const std::string& typeName) const
{
    static const std::vector<std::filesystem::path> s_empty;
    auto it = _histories.find(typeName);
    return it != _histories.end() ? it->second : s_empty;
}

bool FileHistory::empty(const std::string& typeName) const
{
    return entries(typeName).empty();
}

void FileHistory::clear(const std::string& typeName)
{
    auto it = _histories.find(typeName);
    if (it != _histories.end())
    {
        it->second.clear();
    }
}

void FileHistory::clearAll()
{
    for (auto & [typeName, list] : _histories)
    {
        list.clear();
    }
}

}
```

## `app/all.hpp` change

```cpp
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/FileHistory.hpp>   // add after FileDialog
```

## Integration points

- Consumed by `bg2e::ui::FileHistoryWidget` (step 02).
- `FileDialog::imageFilters` (FileDialog.cpp:38) stays as-is for backward
  compatibility; new code uses `FileHistory::get().filtersFor(FileHistory::Image)`
  instead. Optionally, later refactors can derive `imageFilters` from the registry.
- Future custom types: apps can call
  `FileHistory::get().registerType("mymodel", "My models", { "xyz" })` at startup.

## Threading

UI-thread only; singleton is lazily created on first `get()` from the UI thread,
same pattern as `TextureCache::get()`. No mutexes.
