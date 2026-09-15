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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/base.hpp>

#include <string>

namespace bg2e {
namespace ui {

// Vector and matrix value editors
class BG2E_API Vector {
public:
    static bool vec2(
        const std::string& label,
        int * value,
        bool sameLine = false
    );

    static bool vec3(
        const std::string& label,
        int * value,
        bool sameLine = false
    );

    static bool vec4(
        const std::string& label,
        int * value,
        bool sameLine = false
    );

    static bool vec2(
        const std::string& label,
        float * value,
        bool sameLine = false
    );

    static bool vec3(
        const std::string& label,
        float * value,
        bool sameLine = false
    );

    static bool vec4(
        const std::string& label,
        float * value,
        bool sameLine = false
    );

    static bool vec2(
        const std::string& label,
        glm::vec2& value,
        bool sameLine = false
    );

    static bool vec3(
        const std::string& label,
        glm::vec3& value,
        bool sameLine = false
    );

    static bool vec4(
        const std::string& label,
        glm::vec4& value,
        bool sameLine = false
    );

    // Edits a 4x4 transform matrix decomposed as position / rotation (degrees)
    // / scale rows. Rotation is cached internally (keyed by label) to keep the
    // editor stable; if the matrix changes externally, the euler angles are
    // re-extracted.
    static bool mat4(
        const std::string& label,
        glm::mat4& value,
        bool sameLine = false
    );
};

}
}
