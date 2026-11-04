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
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/app/MainLoop.hpp>
#include "detail/IconWidgets.hpp"
#include "detail/IconAccess.hpp"
#include "detail/ImageDraw.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <imgui.h>

namespace bg2e::ui {

namespace {
struct MenuBarTarget {
    ImGuiContext* context;
    ImDrawList* drawList;
};

// Non-owning, paired with beginMenuBar/endMenuBar, including Window's menu bar.
// Identifying its draw list distinguishes bar headers from vertical popups.
thread_local std::vector<MenuBarTarget> menuBars;

class MenuIdentity {
  public:
    explicit MenuIdentity(const void* identity)
    {
        ImGui::PushID(identity);
    }
    ~MenuIdentity()
    {
        ImGui::PopID();
    }
};

class MenuClip {
  public:
    MenuClip(ImDrawList* drawList, const ImVec2& min, const ImVec2& max) : _drawList(drawList)
    {
        _drawList->PushClipRect(min, max, true);
    }
    ~MenuClip()
    {
        _drawList->PopClipRect();
    }

  private:
    ImDrawList* _drawList;
};

// Keep native MenuItem/BeginMenu behavior. The label includes measured blank
// space, so ImGui accounts for the gutter without changing its shortcut/mark
// columns. All siblings in an owned Menu use the same reservation policy.
class MenuIconRow {
  public:
    MenuIconRow(const std::string& id, const std::string& label,
        const std::filesystem::path& path, bool reserveIconSpace)
    {
        if (id.empty())
        {
            throw std::invalid_argument("Icon menus require a nonempty widget ID");
        }
        _icon = detail::resolveIcon(path);
        if (_icon)
        {
            // Registration may throw: do it before BeginMenu can enter a popup.
            detail::imageIdentifier(detail::IconAccess::image(*_icon));
        }
        _target = ImGui::GetWindowDrawList();
        const auto cursor = ImGui::GetCursorScreenPos();
        const auto& style = ImGui::GetStyle();
        const auto context = ImGui::GetCurrentContext();
        const bool horizontal = std::any_of(menuBars.begin(), menuBars.end(),
            [this, context](const MenuBarTarget& bar)
            {
                return bar.context == context && bar.drawList == _target;
            });
        _x = cursor.x + (horizontal ? std::trunc(style.ItemSpacing.x * 0.5f) : 0.0f);
        _size = ImGui::GetTextLineHeight();
        if (reserveIconSpace || !path.empty())
        {
            const auto space = ImGui::CalcTextSize(" ").x;
            if (space <= 0.0f)
            {
                throw std::logic_error("Menu icon gutter requires a font with a space advance");
            }
            const auto gutter = _size + style.ItemInnerSpacing.x;
            _label.assign(size_t(std::ceil(gutter / space)), ' ');
            // CalcTextSize rounds the final width, not each glyph advance.
            // Measure the complete prefix to avoid under-reserving with fonts
            // whose space advances are fractional at the active UI scale.
            while (ImGui::CalcTextSize(_label.c_str()).x < gutter)
            {
                _label.push_back(' ');
            }
        }
        _label += label;
        _label += "###";
        _label += id;
    }

    const char* label() const
    {
        return _label.c_str();
    }

    void paint(bool enabled) const
    {
        if (!_icon || !ImGui::IsItemVisible())
        {
            return;
        }
        // BeginMenu restores its parent's last item but changes the current
        // window on success. Always submit geometry to the captured parent.
        const auto min = ImGui::GetItemRectMin();
        const auto max = ImGui::GetItemRectMax();
        MenuClip clip(_target, min, max);
        auto tint = base::Color::White();
        // Native menu calls have already ended their own disabled scope.
        // Enclosing disabled/style alpha is still applied by shared drawing.
        if (!enabled)
        {
            tint.a *= ImGui::GetStyle().DisabledAlpha;
        }
        detail::paintIcon(*_icon, _x, (min.y + max.y - _size) * 0.5f, _size, tint, _target);
    }

  private:
    std::shared_ptr<Icon> _icon;
    ImDrawList* _target;
    std::string _label;
    float _x;
    float _size;
};
}

void MenuItem::draw() const
{
    draw(!iconPath.empty());
}

void MenuItem::draw(bool reserveIconSpace) const
{
    // The tree owns stable instances while rendering. Visible dynamic labels
    // and icon path changes never contribute to this instance's identity.
    MenuIdentity identity(this);
    const auto text = _labelFn ? _labelFn() : label;
    const bool enabled = !_enabledFn || _enabledFn();
    switch (_type)
    {
    case MenuItemType::Submenu:
        if (Menu::beginIconMenu("entry", text, iconPath, enabled, reserveIconSpace))
        {
            const bool childIcons = std::any_of(_children.begin(), _children.end(),
                [](const MenuItem& item)
                {
                    return item._type != MenuItemType::Separator && !item.iconPath.empty();
                });
            for (const auto& subItem : _children)
            {
                subItem.draw(childIcons);
            }
            Menu::endMenu();
        }
        break;
    case MenuItemType::Action: {
        std::string shortcut = _shortcut.getShortcutString();
        if (Menu::iconMenuItem("entry", text, iconPath, shortcut, false, enabled, reserveIconSpace))
        {
            _shortcut.handler();
        }
        break;
    }
    case MenuItemType::CheckableAction: {
        std::string shortcut = _shortcut.getShortcutString();
        bool checked = _checkedFn ? _checkedFn() : false;
        if (Menu::iconMenuItem("entry", text, iconPath, shortcut, checked, enabled, reserveIconSpace))
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
        for (auto& menuItem : _children)
        {
            menuItem.initShortcuts();
        }
        break;
    case MenuItemType::Action:
        if (_shortcut.key != app::KeyEvent::KeyUnknown)
        {
            auto& shortcuts = app::MainLoop::current()->shortcuts();
            auto shortcut = _shortcut;
            shortcut.handler = [handler = _shortcut.handler, enabled = _enabledFn, allowText = _shortcut.allowInTextInput]
            {
                if ((!enabled || enabled()) && (allowText || !UserInterface::textInputActive()))
                {
                    handler();
                }
            };
            shortcuts.addShortcutMapper(shortcut);
        }
        break;
    case MenuItemType::CheckableAction:
        if (_shortcut.key != app::KeyEvent::KeyUnknown)
        {
            auto& shortcuts = app::MainLoop::current()->shortcuts();
            auto shortcut = _shortcut;
            shortcut.handler = [handler = _shortcut.handler, enabled = _enabledFn, allowText = _shortcut.allowInTextInput]
            {
                if ((!enabled || enabled()) && (allowText || !UserInterface::textInputActive()))
                {
                    handler();
                }
            };
            shortcuts.addShortcutMapper(shortcut);
        }
        break;
    case MenuItemType::Separator:
    default:
        break;
    }
}

bool Menu::beginMenuBar()
{
    if (!ImGui::BeginMenuBar())
    {
        return false;
    }
    menuBars.push_back({ImGui::GetCurrentContext(), ImGui::GetWindowDrawList()});
    return true;
}

bool Menu::beginMenu(const std::string& label, bool enabled)
{
    return ImGui::BeginMenu(label.c_str(), enabled);
}

bool Menu::menuItem(const std::string& label, const std::string& shortcut, bool selected, bool enabled)
{
    return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
}

bool Menu::beginIconMenu(const std::string& id, const std::string& label,
    const std::filesystem::path& iconPath, bool enabled, bool reserveIconSpace)
{
    if (!ImGui::GetCurrentContext())
    {
        return false;
    }
    MenuIconRow row(id, label, iconPath, reserveIconSpace);
    const auto opened = ImGui::BeginMenu(row.label(), enabled);
    row.paint(enabled);
    return opened;
}

bool Menu::iconMenuItem(const std::string& id, const std::string& label,
    const std::filesystem::path& iconPath, const std::string& shortcut,
    bool selected, bool enabled, bool reserveIconSpace)
{
    if (!ImGui::GetCurrentContext())
    {
        return false;
    }
    MenuIconRow row(id, label, iconPath, reserveIconSpace);
    const auto activated = ImGui::MenuItem(row.label(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
    row.paint(enabled);
    return activated;
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
    if (!menuBars.empty() && menuBars.back().context == ImGui::GetCurrentContext())
    {
        menuBars.pop_back();
    }
}

void Menu::draw()
{
    const bool reserveIconSpace = std::any_of(_menuItems.begin(), _menuItems.end(),
        [](const MenuItem& item)
        {
            return item._type != MenuItemType::Separator && !item.iconPath.empty();
        });
    for (auto& item : _menuItems)
    {
        if (!_shortcutInitialized)
        {
            item.initShortcuts();
        }
        item.draw(reserveIconSpace);
    }
    _shortcutInitialized = true;
}

}
