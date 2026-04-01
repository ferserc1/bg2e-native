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
#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>

#include <vector>
#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API Input {
public:
    static bool text(
        const std::string& label,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false
    );
    
    static bool textWithHint(
        const std::string& label,
        const std::string& hint,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false
    );
    
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
    
    static bool colorPicker(
        const std::string& label,
        bg2e::base::Color & color,
        bool sameLine = false
    );
    
    static bool comboBox(
        const std::string& label,
        const std::vector<std::string>& items,
        uint32_t &selected,
        bool sameLine = false,
        bool fitPreview = false
    );
    
    typedef std::function<void(std::vector<std::string>&)> ItemListCallback;
    
    static bool comboBox(
        const std::string& label,
        ItemListCallback itemsCb,
        uint32_t &selected,
        bool sameLine = false,
        bool fitPreview = false
    ) {
        std::vector<std::string> items;
        itemsCb(items);
        return Input::comboBox(label, items, selected, sameLine, fitPreview);
    }
    
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
};

}
}
