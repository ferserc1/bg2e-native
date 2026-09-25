# Step 02 — `bg2e::ui::FileHistoryWidget`

## Files

- Create `lib/include/bg2e/ui/FileHistoryWidget.hpp`
- Create `lib/src/bg2e/ui/FileHistoryWidget.cpp`
- Modify `lib/include/bg2e/ui/all.hpp` (add include)

## Design

Reusable ImGui picker bound to a `FileHistory` type. It encapsulates the complete
flow so callers **never** touch `FileDialog` or `FileHistory` directly:

- On trigger click → `openPicker()`:
  - History empty → native dialog via
    `FileDialog::getOpenFilePath(FileHistory::get().filtersFor(type))`.
  - History non-empty → `ImGui` popup anchored at the widget:
    - Fixed (outside scroll): **"Open file..."** button → native dialog.
    - Scrollable `ImGui::BeginChild` region: one selectable row per history entry.
      Rows show a 32×32 thumbnail (if a `ThumbnailProvider` is set and returns a
      valid `ImTextureID`) + file name; full path as tooltip.
- Any resulting path goes through `commitSelection()`, which calls
  `FileHistory::get().add(type, path)` and stores it in `_selectedPath`.
- `draw*()` returns `true` on the frame a file was selected; the caller reads the
  path via `selectedPath()`.

ImGui popup state is keyed by a stable ID derived from the widget's `id` parameter,
so multiple widgets on screen don't collide (same `##` ID conventions used in
`TextureWidgets::selectTexture`).

> **Header rule**: `imgui.h` must never be included from engine headers. Textures
> cross this API as the opaque `TextureID` (`uint64_t`, layout-compatible with
> `ImTextureID` and `VkDescriptorSet`); casts to ImGui types happen only in the
> `.cpp`.

## Header (`lib/include/bg2e/ui/FileHistoryWidget.hpp`)

```cpp
#pragma once

#include <bg2e/common.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace bg2e {
namespace ui {

class BG2E_API FileHistoryWidget {
public:
    // Opaque texture identifier, so this header does not expose imgui.h.
    // Compatible with ImTextureID and VkDescriptorSet (both are 64-bit values).
    // A value of 0 means "no texture available".
    using TextureID = uint64_t;

    // Returns 0 when no thumbnail is available
    using ThumbnailProvider = std::function<TextureID(const std::filesystem::path&)>;

    explicit FileHistoryWidget(const std::string& historyType);

    // Draws the trigger as a text button.
    // Returns true on the frame a file was selected (read selectedPath()).
    bool draw(const std::string& id, const std::string& buttonLabel = "Select file...");

    // Draws the trigger as an image button (e.g. current texture preview).
    bool drawImageButton(const std::string& id, TextureID image, uint32_t width, uint32_t height);

    inline const std::filesystem::path& selectedPath() const { return _selectedPath; }

    // Optional: if set, history rows show thumbnails next to the file name.
    // If unset (or the provider returns 0 for a path) rows show name only.
    inline void setThumbnailProvider(ThumbnailProvider p) { _thumbnailProvider = p; }

    inline const std::string& historyType() const { return _historyType; }

protected:
    std::string _historyType;
    std::filesystem::path _selectedPath;
    ThumbnailProvider _thumbnailProvider;

    std::string _popupId;   // set by draw*() before openPicker()

    void openPicker();
    void commitSelection(const std::filesystem::path& path);
    bool drawHistoryPopup();   // returns true when a selection was committed
};

}
}
```

## Implementation (`lib/src/bg2e/ui/FileHistoryWidget.cpp`)

```cpp
#include <bg2e/ui/FileHistoryWidget.hpp>
#include <bg2e/app/FileHistory.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/ui/Button.hpp>

#include "imgui.h"

namespace bg2e::ui {

FileHistoryWidget::FileHistoryWidget(const std::string& historyType)
    : _historyType{ historyType }
{
}

bool FileHistoryWidget::draw(const std::string& id, const std::string& buttonLabel)
{
    _selectedPath.clear();
    _popupId = "fileHistory##" + id;
    if (Button::button(buttonLabel + "##" + id))
    {
        openPicker();
    }
    return drawHistoryPopup();
}

bool FileHistoryWidget::drawImageButton(const std::string& id, TextureID image,
                                        uint32_t width, uint32_t height)
{
    _selectedPath.clear();
    _popupId = "fileHistory##" + id;

    bool clicked = false;
    if (image != 0)
    {
        clicked = ImGui::ImageButton(
            id.c_str(), static_cast<ImTextureID>(image),
            ImVec2(static_cast<float>(width), static_cast<float>(height)),
            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else
    {
        // No preview available: fall back to a text button of similar footprint
        clicked = Button::button("...##" + id);
    }
    if (clicked)
    {
        openPicker();
    }
    return drawHistoryPopup();
}

void FileHistoryWidget::openPicker()
{
    auto & history = app::FileHistory::get();
    if (history.empty(_historyType))
    {
        // No history yet: go straight to the native file dialog
        auto filePath = app::FileDialog::getOpenFilePath(history.filtersFor(_historyType));
        if (!filePath.empty())
        {
            commitSelection(filePath);
        }
    }
    else
    {
        ImGui::OpenPopup(_popupId.c_str());
    }
}

bool FileHistoryWidget::drawHistoryPopup()
{
    bool committed = !_selectedPath.empty();   // may be set by openPicker() this frame

    if (ImGui::BeginPopup(_popupId.c_str()))
    {
        auto & history = app::FileHistory::get();

        // Fixed area: always visible, outside the scroll region
        if (Button::button("Open file...##" + _popupId))
        {
            auto filePath = app::FileDialog::getOpenFilePath(history.filtersFor(_historyType));
            if (!filePath.empty())
            {
                commitSelection(filePath);
                committed = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::Separator();

        // Scrollable history list
        if (ImGui::BeginChild(("##scroll" + _popupId).c_str(),
                              ImVec2(360.0f, 240.0f), true))
        {
            for (const auto & entry : history.entries(_historyType))
            {
                ImGui::PushID(entry.string().c_str());

                auto thumb = _thumbnailProvider ? _thumbnailProvider(entry) : 0;
                if (thumb != 0)
                {
                    ImGui::Image(static_cast<ImTextureID>(thumb), ImVec2(32.0f, 32.0f));
                    ImGui::SameLine();
                }

                if (ImGui::Selectable(entry.filename().string().c_str()))
                {
                    commitSelection(entry);
                    committed = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", entry.string().c_str());
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }
    return committed;
}

void FileHistoryWidget::commitSelection(const std::filesystem::path& path)
{
    app::FileHistory::get().add(_historyType, path);
    _selectedPath = path;
}

}
```

## `ui/all.hpp` change

```cpp
#include <bg2e/ui/TextureWidgets.hpp>
#include <bg2e/ui/FileHistoryWidget.hpp>   // add
```

## Integration points

- Used by `TextureWidgets` (step 03) with type `FileHistory::Image` and a
  `TextureCache`-backed `ThumbnailProvider`.
- Usable standalone for any file type: with no `ThumbnailProvider`, rows show the
  file name only (satisfies the "works autonomously for non-images" requirement).
- Centralizes file-type knowledge: filters always come from
  `FileHistory::filtersFor()`, never hardcoded in UI code.

## Details / edge cases

- **Popup vs dialog same-frame**: when history is empty, `openPicker()` runs the
  modal native dialog synchronously and commits immediately; `drawHistoryPopup()`
  detects the non-empty `_selectedPath` and returns `true` without opening a popup.
- **Selection commit timing**: `commitSelection()` adds to history even when picking
  from the list (moves the entry to the MRU front) — intended behavior.
- **Window size**: 360×240 child with border; tweak after visual review.
- **Thumbnail failure**: provider returning `0` (deleted file, unsupported, or no
  engine) renders the row without an image — no crash path.
