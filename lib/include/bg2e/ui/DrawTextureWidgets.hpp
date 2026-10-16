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
#include <filesystem>
#include <memory>
#include <string>

namespace bg2e::draw {
class Texture;
}
namespace bg2e::ui {
class UserInterface;

// Rendering only: selected paths/clear intent are returned for safe queued edits.
// File history lists names; it does not load thumbnail files during UI draw.
class BG2E_API DrawTextureWidgets {
  public:
    struct Result {
        bool selected = false;
        bool cleared = false;
        std::filesystem::path path;
    };

    DrawTextureWidgets();
    ~DrawTextureWidgets();
    DrawTextureWidgets(const DrawTextureWidgets&) = delete;
    DrawTextureWidgets& operator=(const DrawTextureWidgets&) = delete;

    Result draw(UserInterface& ui, const std::string& label, const std::shared_ptr<draw::Texture>& texture);
    void cleanup();

  private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}
