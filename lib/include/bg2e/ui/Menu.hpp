//
//  Menu.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/Shortcuts.hpp>

#include <string>
#include <vector>
#include <functional>

namespace bg2e {
namespace ui {

enum MenuItemType {
    Action,
    Separator,
    Submenu
};

class BG2E_API MenuItem {
public:
    MenuItem() :label {}, _type { MenuItemType::Separator } {}
    MenuItem(const std::string& l) :label { l }, _type { MenuItemType::Separator } {}
    MenuItem(const std::string& l, const app::Shortcuts::ShortcutData& sc)
        :label { l }, _type { MenuItemType::Action }, _shortcut {sc} {}

    std::string label;

    void addMenuItem(const MenuItem & item)
    {
        _type = MenuItemType::Submenu;
        _children.push_back(item);
    }

    void draw() const;

    void initShortcuts() const;

protected:
    std::vector<MenuItem> _children;
    MenuItemType _type = MenuItemType::Separator;
    app::Shortcuts::ShortcutData _shortcut {};

    bool _shortcutInitialized = false;
};

class BG2E_API Menu {
public:
    static bool beginMenuBar();
    
    static bool beginMenu(
        const std::string& label,
        bool enabled = true
    );
    
    static bool menuItem(
        const std::string & label,
        const std::string & shortcut = "",
        bool selected = false,
        bool enabled = true
    );
    
    static void separator();
    
    static void endMenu();
    
    static void endMenuBar();

    void addMenuItem(const MenuItem & item)
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
