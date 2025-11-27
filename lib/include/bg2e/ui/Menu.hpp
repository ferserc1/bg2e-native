//
//  Menu.hpp

#pragma once

#include <bg2e/common.hpp>

#include <string>

namespace bg2e {
namespace ui {

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
};

}
}
