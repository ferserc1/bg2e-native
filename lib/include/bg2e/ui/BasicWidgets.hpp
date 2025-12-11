#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace ui {

class BG2E_API BasicWidgets {
public:
    // Use a negative value to align from right side
    static void sameLine(int32_t xPos = 0);
    
    static void text(const std::string & text, bool sameLine = false);
    static void separator(const std::string & title = "", bool sameLine = false);
    static void listItem(const std::string & label, bool sameLine = false);
    
    static bool button(const std::string & title, bool sameLine = false, bool disabled = false);
    static bool checkBox(const std::string & title, bool * value = nullptr, bool sameLine = false, bool disabled = false);
    static bool radioButton(const std::string & label, int * value = nullptr, int id = 0, bool sameLine = false, bool disabled = false);
    
    static bool beginTree(const std::string & label);
    static void endTree();
    static bool collapsingHeader(const std::string & title, bool visible = true);
    
    static uint32_t calcTextWidth(const std::string & title);
    static uint32_t calcTextHeight(const std::string & title);
    static uint32_t calcButtonWidth(const std::string & title);
    static uint32_t calcButtonHeight(const std::string & title);
    static uint32_t getItemHorizontalSpacing();
    static uint32_t getItemVerticalSpacing();

    static void padding(uint32_t width, uint32_t height);
    
};

}
}
