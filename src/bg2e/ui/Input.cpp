
#include <bg2e/ui/Input.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void Input::text(const std::string& label, std::string& value, int maxLength, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    value.reserve(maxLength);
    ImGui::InputText(label.c_str(), value.data(), maxLength);
}

void Input::textWithHint(
    const std::string& label,
    const std::string& hint,
    std::string& value,
    int maxLength,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    value.reserve(maxLength);
    ImGui::InputTextWithHint(label.c_str(), hint.c_str(), value.data(), maxLength);
}

void Input::number(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputInt(label.c_str(), value);
}

void Input::number(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputFloat(label.c_str(), value);
}

void Input::number(
    const std::string& label,
    double * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputDouble(label.c_str(), value);
}

void Input::vec2(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputInt2(label.c_str(), value);
}

void Input::vec3(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputInt3(label.c_str(), value);
}

void Input::vec4(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputInt4(label.c_str(), value);
}

void Input::vec2(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputFloat2(label.c_str(), value);
}

void Input::vec3(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputFloat3(label.c_str(), value);
}

void Input::vec4(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::InputFloat4(label.c_str(), value);
}

}
}
