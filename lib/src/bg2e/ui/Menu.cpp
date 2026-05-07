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

#include <bg2e/ui/Menu.hpp>
#include <bg2e/app/MainLoop.hpp>

#include "imgui.h"

namespace bg2e::ui {

void MenuItem::draw() const
{
    switch (_type) {
    case MenuItemType::Submenu:
        if (Menu::beginMenu(label))
        {
            for (auto & subItem : _children)
            {
                subItem.draw();
            }
            Menu::endMenu();
        }
        break;
    case MenuItemType::Action:
        {
            std::string shortcut = _shortcut.getShortcutString();
            if (Menu::menuItem(label, shortcut))
            {
                _shortcut.handler();
            }
            break;
        }
    case MenuItemType::CheckableAction:
        {
            std::string shortcut = _shortcut.getShortcutString();
            bool checked = _checkedFn ? _checkedFn() : false;
            if (Menu::menuItem(label, shortcut, checked))
            {
                _shortcut.handler();
            }
            break;
        }
    case MenuItemType::Separator:
        Menu::separator();
        break;
    }
}

void MenuItem::initShortcuts() const
{
    switch (_type)
    {
    case MenuItemType::Submenu:
        for (auto &menuItem : _children)
        {
            menuItem.initShortcuts();
        }
        break;
    case MenuItemType::Action:
        if (_shortcut.key != app::KeyEvent::KeyUnknown)
        {
            auto & shortcuts = app::MainLoop::current()->shortcuts();
            shortcuts.addShortcutMapper(_shortcut);
        }
        break;
    case MenuItemType::CheckableAction:
        if (_shortcut.key != app::KeyEvent::KeyUnknown)
        {
            auto & shortcuts = app::MainLoop::current()->shortcuts();
            shortcuts.addShortcutMapper(_shortcut);
        }
        break;
    case MenuItemType::Separator:
    default:
        break;
    }
}

bool Menu::beginMenuBar()
{
    return ImGui::BeginMenuBar();
}

bool Menu::beginMenu(const std::string& label, bool enabled)
{
    return ImGui::BeginMenu(label.c_str(), enabled);
}

bool Menu::menuItem(const std::string& label, const std::string& shortcut, bool selected, bool enabled)
{
    return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
}

void Menu::separator()
{
    ImGui::Separator();
}

void Menu::endMenu()
{
    ImGui::EndMenu();
}

void Menu::endMenuBar()
{
    ImGui::EndMenuBar();
}

void Menu::draw()
{
    for (auto & item : _menuItems)
    {
        if (!_shortcutInitialized)
        {
            item.initShortcuts();
        }
        item.draw();
    }
    _shortcutInitialized = true;
}

}
