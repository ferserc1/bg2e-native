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

#include <string>

namespace bg2e {
namespace ui {

// Scalar numeric value editors (input, slider and drag widgets)
class BG2E_API Numeric {
public:
    static bool number(
        const std::string& label,
        int * value,
        bool sameLine = false
    );

    static bool number(
        const std::string& label,
        float * value,
        bool sameLine = false
    );

    static bool number(
        const std::string& label,
        double * value,
        bool sameLine = false
    );

    static bool slider(
        const std::string& label,
        int * value,
        int min = 0,
        int max = 100,
        bool sameLine = false
    );

    static bool slider(
        const std::string& label,
        float * value,
        float min = 0.0f,
        float max = 1.0f,
        bool sameLine = false
    );

    static bool sliderInt(
        const std::string& label,
        int * value,
        int min = 0,
        int max = 255,
        bool sameLine = false
    );

    static bool sliderFloat(
        const std::string& label,
        float * value,
        float min = 0.0f,
        float max = 1.0f,
        bool sameLine = false
    );

    static bool sliderDouble(
        const std::string& label,
        double * value,
        double min = 0.0,
        double max = 1.0,
        bool sameLine = false
    );

    // min == max == 0 means unclamped range (standard ImGui convention)
    static bool drag(
        const std::string& label,
        float * value,
        float speed = 0.1f,
        float min = 0.0f,
        float max = 0.0f,
        bool sameLine = false
    );

    // min == max == 0 means unclamped range (standard ImGui convention)
    static bool drag(
        const std::string& label,
        int * value,
        float speed = 1.0f,
        int min = 0,
        int max = 0,
        bool sameLine = false
    );
};

}
}
