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
#include <bg2e/app/Shortcuts.hpp>

#include <filesystem>
#include <string>
#include <vector>
#include <functional>
#include <utility>

namespace bg2e {
namespace ui {

enum MenuItemType
{
    Action,
    CheckableAction,
    Separator,
    Submenu
};

class BG2E_API MenuItem {
  public:
    MenuItem() : label{}, _type{MenuItemType::Separator}
    {
    }
    MenuItem(const std::string& l) : label{l}, _type{MenuItemType::Separator}
    {
    }
    MenuItem(const std::string& l, const app::Shortcuts::ShortcutData& sc)
        : label{l}, _type{MenuItemType::Action}, _shortcut{sc}
    {
    }

    MenuItem(const std::string& l, const app::Shortcuts::ShortcutData& sc,
             std::function<bool()> isChecked)
        : label{l}, _type{MenuItemType::CheckableAction}, _shortcut{sc}, _checkedFn(isChecked)
    {
    }

    MenuItem(const std::string& l, const app::Shortcuts::ShortcutData& sc,
        std::function<std::string()> dynamicLabel, std::function<bool()> enabled)
        : label(l), _type(MenuItemType::Action), _shortcut(sc),
          _labelFn(std::move(dynamicLabel)), _enabledFn(std::move(enabled))
    {
    }

    std::string label;
    // Empty means no icon; sibling rows reserve a gutter if any has an icon.
    std::filesystem::path iconPath;

    void addMenuItem(const MenuItem& item)
    {
        _type = MenuItemType::Submenu;
        _children.push_back(item);
    }

    void draw() const;

    void initShortcuts() const;

  private:
    friend class Menu;
    void draw(bool reserveIconSpace) const;

  protected:
    std::vector<MenuItem> _children;
    MenuItemType _type = MenuItemType::Separator;
    app::Shortcuts::ShortcutData _shortcut{};
    std::function<bool()> _checkedFn = nullptr;
    std::function<std::string()> _labelFn;
    std::function<bool()> _enabledFn;

    bool _shortcutInitialized = false;
};

class BG2E_API Menu {
  public:
    static bool beginMenuBar();

    static bool beginMenu(
        const std::string& label,
        bool enabled = true);

    static bool menuItem(
        const std::string& label,
        const std::string& shortcut = "",
        bool selected = false,
        bool enabled = true);

    // Explicit identity is independent of path and dynamic label. Labels retain
    // normal ImGui ## display semantics. Draw within a UI menu/window frame.
    // Icon size follows the current text line height (already scaled).
    // Empty paths reserve an aligned gutter by default. Use the same setting
    // for sibling rows; a nonempty path always reserves its icon space.
    static bool beginIconMenu(const std::string& id, const std::string& label,
        const std::filesystem::path& iconPath, bool enabled = true, bool reserveIconSpace = true);
    static bool iconMenuItem(const std::string& id, const std::string& label,
        const std::filesystem::path& iconPath, const std::string& shortcut = "",
        bool selected = false, bool enabled = true, bool reserveIconSpace = true);

    static void separator();

    static void endMenu();

    static void endMenuBar();

    void addMenuItem(const MenuItem& item)
    {
        _menuItems.push_back(item);
    }

    void draw();

  protected:
    std::vector<MenuItem> _menuItems;
    bool _shortcutInitialized = false;
};

}
}
