
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

}
}
