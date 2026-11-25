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

#include <bg2e/app/Shortcuts.hpp>
#include <bg2e/common.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace bg2e::ui {

// Share a const definition between surfaces. Callbacks read the current
// application state, and the ID remains stable across application versions.
struct BG2E_API ActionDefinition {
    std::string id;
    std::string label;
    std::filesystem::path iconPath;
    std::string tooltip;
    std::function<void()> execute;
    std::function<std::string()> dynamicLabel;
    std::function<bool()> enabled;
    std::function<bool()> checked;
    // Key and modifier metadata. Activation always uses execute.
    app::Shortcuts::ShortcutData shortcut;
    // Menu parents separated by '|'. An empty path keeps the action toolbar-only.
    std::string path;

    std::string displayLabel() const;
    bool isEnabled() const;
    bool isChecked() const;

    void invoke() const;

    // Toolbar registers shortcuts automatically. Other owners can register
    // explicitly. A shared action is registered once per Shortcuts instance.
    static void registerShortcut(app::Shortcuts& shortcuts,
        const std::shared_ptr<const ActionDefinition>& action);
};

}
