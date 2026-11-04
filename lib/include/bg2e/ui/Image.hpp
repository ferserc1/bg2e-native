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
#include <bg2e/ui/Icon.hpp>
#include <bg2e/ui/IconDrawOptions.hpp>
#include <filesystem>
#include <memory>

namespace bg2e::ui {

class BG2E_API Image {
  public:
    // Draw inside a square of positive, finite logical size, scaled once by
    // UserInterface::getScale(). Preserve source aspect ratio and center it.
    // Call inside a UI window/frame. Empty path/null resource draws nothing.
    // No active session or stale/cross-session resource: no-op, without I/O.
    static void icon(const std::filesystem::path& path, float size = 20.0f,
        const IconDrawOptions& options = {});
    static void icon(const std::shared_ptr<Icon>& resource, float size = 20.0f,
        const IconDrawOptions& options = {});
};

}
