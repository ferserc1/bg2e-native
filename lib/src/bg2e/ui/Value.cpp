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

#include <bg2e/ui/Value.hpp>

#include <cstring>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Value::text(const std::string& label, std::string& value, int maxLength, bool sameLine)
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

bool Value::textWithHint(
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

bool Value::colorPicker(
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

bool Value::comboBox(
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
    if (selected < 0)
    {
        selected = 0;
    }
    if (selected >= items.size())
    {
        selected = items.size() - 1;
    }
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

bool Value::comboBox(
    const std::vector<std::string>& items,
    uint32_t &selected,
    const std::string& id,
    bool sameLine,
    bool fitPreview
) {
    return Value::comboBox("##" + id, items, selected, sameLine, fitPreview);
}

}
}
