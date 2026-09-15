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

#include <bg2e/ui/Vector.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <unordered_map>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Vector::vec2(
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

bool Vector::vec3(
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

bool Vector::vec4(
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

bool Vector::vec2(
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

bool Vector::vec3(
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

bool Vector::vec4(
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

bool Vector::vec2(
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

bool Vector::vec3(
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

bool Vector::vec4(
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

bool Vector::mat4(
    const std::string& label,
    glm::mat4& value,
    bool /* sameLine */
) {
    struct Mat4Cache {
        glm::mat4 lastMatrix;
        glm::vec3 eulerDegrees;
    };
    static std::unordered_map<std::string, Mat4Cache> s_cache;

    auto cacheIt = s_cache.find(label);
    if (cacheIt == s_cache.end() || cacheIt->second.lastMatrix != value)
    {
        // New widget or external change: re-extract the euler angles
        glm::vec3 scale;
        scale.x = glm::length(glm::vec3(value[0]));
        scale.y = glm::length(glm::vec3(value[1]));
        scale.z = glm::length(glm::vec3(value[2]));

        glm::mat3 rot;
        if (scale.x != 0.0f) rot[0] = glm::vec3(value[0]) / scale.x;
        else rot[0] = glm::vec3(1.0f, 0.0f, 0.0f);
        if (scale.y != 0.0f) rot[1] = glm::vec3(value[1]) / scale.y;
        else rot[1] = glm::vec3(0.0f, 1.0f, 0.0f);
        if (scale.z != 0.0f) rot[2] = glm::vec3(value[2]) / scale.z;
        else rot[2] = glm::vec3(0.0f, 0.0f, 1.0f);

        float xr, yr, zr;
        glm::extractEulerAngleXYZ(glm::mat4(rot), xr, yr, zr);

        Mat4Cache cache;
        cache.lastMatrix = value;
        cache.eulerDegrees = glm::degrees(glm::vec3(xr, yr, zr));
        cacheIt = s_cache.insert_or_assign(label, cache).first;
    }

    glm::vec3 pos = glm::vec3(value[3]);
    glm::vec3 eulerDeg = cacheIt->second.eulerDegrees;
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(value[0]));
    scale.y = glm::length(glm::vec3(value[1]));
    scale.z = glm::length(glm::vec3(value[2]));

    bool changed = false;
    if (vec3(("Position##" + label).c_str(), pos)) changed = true;
    if (vec3(("Rotation##" + label).c_str(), eulerDeg)) changed = true;
    if (vec3(("Scale##" + label).c_str(), scale)) changed = true;

    if (changed)
    {
        cacheIt->second.eulerDegrees = eulerDeg;
        glm::vec3 rad = glm::radians(eulerDeg);
        value = glm::translate(glm::mat4(1.0f), pos)
            * glm::eulerAngleXYZ(rad.x, rad.y, rad.z)
            * glm::scale(glm::mat4(1.0f), scale);
        cacheIt->second.lastMatrix = value;
    }

    return changed;
}

}
}
