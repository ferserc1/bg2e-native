/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
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
#include <memory>
#include <cstdint>
#include <string>

namespace bg2e::ui {
namespace detail {
struct PreviewEntry;
class PreviewRegistry;
}

// Session-scoped opaque preview. Registration/resources survive submitted UI
// draws and are retired after GPU completion. Invalid after UI cleanup.
class BG2E_API TexturePreview {
  public:
    bool valid() const;
    void drawImage(uint32_t width, uint32_t height) const;
    bool imageButton(const std::string& id, uint32_t width, uint32_t height) const;

  private:
    friend class detail::PreviewRegistry;
    explicit TexturePreview(std::shared_ptr<detail::PreviewEntry> entry);
    std::shared_ptr<detail::PreviewEntry> _entry;
};

}
