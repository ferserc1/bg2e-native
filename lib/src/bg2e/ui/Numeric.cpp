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

#include <bg2e/ui/Numeric.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Numeric::number(
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

bool Numeric::number(
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

bool Numeric::number(
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

bool Numeric::slider(
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

bool Numeric::slider(
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

bool Numeric::sliderInt(
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

bool Numeric::sliderFloat(
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

bool Numeric::sliderDouble(
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

bool Numeric::drag(
    const std::string& label,
    float * value,
    float speed,
    float min,
    float max,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::DragFloat(label.c_str(), value, speed, min, max);
}

bool Numeric::drag(
    const std::string& label,
    int * value,
    float speed,
    int min,
    int max,
    bool sameLine
) {
    if (sameLine)
    {
        ImGui::SameLine();
    }
    return ImGui::DragInt(label.c_str(), value, speed, min, max);
}

}
}
