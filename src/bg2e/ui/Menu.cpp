//
//  Menu.cpp

#include <bg2e/ui/Menu.hpp>
#include "imgui.h"

namespace bg2e::ui {

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

}
