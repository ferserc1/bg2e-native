
#include <bg2e/ui/BasicWidgets.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void BasicWidgets::sameLine(int32_t xPos)
{
    if (xPos < 0)
    {
        xPos = std::abs(xPos);
        auto winWidth = ImGui::GetWindowWidth();
        ImGui::SameLine(winWidth - static_cast<float>(xPos));
    }
    else {
        ImGui::SameLine(static_cast<float>(xPos));
    }
}

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

bool BasicWidgets::button(const std::string & title, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::Button(title.c_str());
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

bool BasicWidgets::checkBox(const std::string & title, bool * value, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::Checkbox(title.c_str(), value);
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

bool BasicWidgets::radioButton(const std::string & label, int * value, int id, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::RadioButton(label.c_str(), value, id);
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

bool BasicWidgets::beginTree(const std::string & label)
{
    return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
}

void BasicWidgets::endTree()
{
    ImGui::TreePop();
}

bool BasicWidgets::collapsingHeader(const std::string & title, bool visible)
{
    return ImGui::CollapsingHeader(title.c_str(), visible ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

uint32_t BasicWidgets::calcTextWidth(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    return static_cast<uint32_t>(textSize.x);
}

uint32_t BasicWidgets::calcTextHeight(const std::string & title)
{
ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
return static_cast<uint32_t>(textSize.y);
}

uint32_t BasicWidgets::calcButtonWidth(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    return static_cast<uint32_t>(textSize.x + padding.x * 2.0f);
}

uint32_t BasicWidgets::calcButtonHeight(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    return static_cast<uint32_t>(textSize.y + padding.y * 2.0f);
}

uint32_t BasicWidgets::getItemHorizontalSpacing()
{
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    return static_cast<uint32_t>(spacing.x);
}

uint32_t BasicWidgets::getItemVerticalSpacing()
{
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    return static_cast<uint32_t>(spacing.y);
}

void BasicWidgets::padding(uint32_t width, uint32_t height)
{
    ImGui::Dummy(ImVec2(static_cast<float>(width), static_cast<float>(height)));
}

}
}
