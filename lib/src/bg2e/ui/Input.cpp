
#include <bg2e/ui/Input.hpp>
#include <iostream>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Input::text(const std::string& label, std::string& value, int maxLength, bool sameLine)
{
    char * stringValue = new char[maxLength];
    strcpy(stringValue, value.c_str());
    if (sameLine)
    {
        ImGui::SameLine();
    }
    if (ImGui::InputText(label.c_str(), stringValue, maxLength))
    {
        value = stringValue;
        return true;
    }
    delete [] stringValue;
    return false;
}

bool Input::textWithHint(
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
    return ImGui::InputTextWithHint(label.c_str(), hint.c_str(), value.data(), maxLength);
}

bool Input::number(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputInt(label.c_str(), value);
}

bool Input::number(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputFloat(label.c_str(), value);
}

bool Input::number(
    const std::string& label,
    double * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputDouble(label.c_str(), value);
}

bool Input::vec2(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputInt2(label.c_str(), value);
}

bool Input::vec3(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputInt3(label.c_str(), value);
}

bool Input::vec4(
    const std::string& label,
    int * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputInt4(label.c_str(), value);
}

bool Input::vec2(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputFloat2(label.c_str(), value);
}

bool Input::vec3(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputFloat3(label.c_str(), value);
}

bool Input::vec4(
    const std::string& label,
    float * value,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::InputFloat4(label.c_str(), value);
}

bool Input::vec2(
    const std::string& label,
    glm::vec2& value,
    bool sameLine
) {
    float inValue[] { value.x, value.y };
    if (vec2(label, inValue, sameLine))
    {
        value.x = inValue[0];
        value.y = inValue[1];
        return true;
    }
    return false;
}

bool Input::vec3(
    const std::string& label,
    glm::vec3& value,
    bool sameLine
) {
    float inValue[] { value.x, value.y, value.z };
    if (vec3(label, inValue, sameLine))
    {
        value.x = inValue[0];
        value.y = inValue[1];
        value.z = inValue[2];
        return true;
    }
    return false;
}

bool Input::vec4(
    const std::string& label,
    glm::vec4& value,
    bool sameLine
) {
    float inValue[] { value.x, value.y, value.z, value.w };
    if (vec4(label, inValue, sameLine))
    {
        value.x = inValue[0];
        value.y = inValue[1];
        value.z = inValue[2];
        value.w = inValue[3];
        return true;
    }
    return false;
}

bool Input::slider(
    const std::string& label,
    int * value,
    int min,
    int max,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::SliderInt(label.c_str(), value, min, max);
}

bool Input::slider(
    const std::string& label,
    float * value,
    float min,
    float max,
    bool sameLine
)  {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::SliderFloat(label.c_str(), value, min, max);
}

bool Input::colorPicker(
    const std::string& label,
    bg2e::base::Color& color,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    float col[] = { color.r, color.g, color.b, color.a };
    bool result = ImGui::ColorEdit4(label.c_str(), col);
    color.r = col[0];
    color.g = col[1];
    color.b = col[2];
    color.a = col[3];
    return result;
}

bool Input::comboBox(
    const std::string& label,
    const std::vector<std::string>& items,
    uint32_t &selected,
    bool sameLine,
    bool fitPreview
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    
    ImGuiComboFlags flags = 0;
    if (fitPreview)
    {
        flags |= ImGuiComboFlags_WidthFitPreview;
    }
    bool changed = false;
    auto selectedLabel = std::to_string(selected) + ": " + items[selected];
    if (ImGui::BeginCombo(label.c_str(), selectedLabel.c_str(), flags))
    {
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(items.size()); ++idx)
        {
            const bool isSelected = selected == idx;
            auto label = std::to_string(idx) + ": " + items[idx];
            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selected = idx;
                changed = true;
            }
            
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        
        
        ImGui::EndCombo();
    }
    
    return changed;
}

bool Input::sliderInt(
    const std::string& label,
    int * value,
    int min,
    int max,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::SliderInt(label.c_str(), value, min, max);
}

bool Input::sliderFloat(
    const std::string& label,
    float * value,
    float min,
    float max,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::SliderFloat(label.c_str(), value, min, max);
}

bool Input::sliderDouble(
    const std::string& label,
    double * value,
    double min,
    double max,
    bool /* sameLine */
) {
    return sliderFloat(
        label,
        reinterpret_cast<float*>(value),
        static_cast<float>(min),
        static_cast<float>(max)
    );
}
    

}
}
