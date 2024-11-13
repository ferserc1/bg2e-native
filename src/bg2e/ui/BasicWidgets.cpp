
#include <bg2e/ui/BasicWidgets.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void BasicWidgets::text(const std::string & text, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::Text("%s", text.c_str());
}

void BasicWidgets::separator(const std::string & title, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::SeparatorText(title.c_str());
}

void BasicWidgets::listItem(const std::string & label, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::BulletText("%s", label.c_str());
}

bool BasicWidgets::button(const std::string & title, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::Button(title.c_str());
}

void BasicWidgets::checkBox(const std::string & title, bool * value, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::Checkbox(title.c_str(), value);
}

void BasicWidgets::radioButton(const std::string & label, int * value, int id, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::RadioButton(label.c_str(), value, id);
}

}
}
