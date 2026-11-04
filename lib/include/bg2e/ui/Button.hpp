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

#include <string>

namespace bg2e {
namespace ui {

// Button family widgets wrappers (command, check and radio buttons)
class BG2E_API Button {
  public:
    static bool button(const std::string& title, bool sameLine = false, bool disabled = false);

    // Explicit stable ID, independent of path and label. Size is a positive,
    // finite logical square, scaled once, with source aspect ratio preserved.
    // Call inside a UI window/frame. Empty path/null icon omits the icon slot.
    // No active session or stale/cross-session resource: false without drawing.
    // A missing source uses a fallback, with the same size and widget identity.
    // Labels are literal visible text (## has no special meaning).
    static bool iconButton(const std::string& id, const std::filesystem::path& path,
        float size = 20.0f, const IconDrawOptions& options = {});
    static bool iconButton(const std::string& id, const std::shared_ptr<Icon>& icon,
        float size = 20.0f, const IconDrawOptions& options = {});
    static bool iconTextButton(const std::string& id, const std::string& label,
        const std::filesystem::path& path, float size = 20.0f, const IconDrawOptions& options = {});
    static bool iconTextButton(const std::string& id, const std::string& label,
        const std::shared_ptr<Icon>& icon, float size = 20.0f, const IconDrawOptions& options = {});

    static bool checkBox(const std::string& title, bool* value = nullptr, bool sameLine = false, bool disabled = false);
    static bool radioButton(const std::string& label, int* value = nullptr, int id = 0, bool sameLine = false, bool disabled = false);
};

}
}
