/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation: either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

    using ThumbnailProvider = std::function<TextureID(const std::filesystem::path&)>;

    explicit FileHistoryWidget(const std::string& historyType);

    bool draw(const std::string& id, const std::string& buttonLabel = "Select file...");
    bool drawImageButton(const std::string& id, TextureID image, uint32_t width, uint32_t height);

    inline const std::filesystem::path& selectedPath() const { return _selectedPath; }

    inline void setThumbnailProvider(ThumbnailProvider provider)
    {
        _thumbnailProvider = provider;
    }

    inline const std::string& historyType() const { return _historyType; }

protected:
    std::string _historyType;
    std::filesystem::path _selectedPath;
    ThumbnailProvider _thumbnailProvider;
    std::string _popupId;

    void openPicker();
    void commitSelection(const std::filesystem::path& path);
    bool drawHistoryPopup();
};

}
}
