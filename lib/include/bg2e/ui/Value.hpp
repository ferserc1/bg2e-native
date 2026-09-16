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

#include <functional>
#include <string>
#include <vector>

namespace bg2e {
namespace ui {

// Non-numeric value editors (text fields, color picker and combo boxes)
class BG2E_API Value {
public:
    static bool text(
        const std::string& label,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false
    );

    // ID-only text input: no visible label on the left, the id parameter
    // is only used as ImGui identifier. Pass readOnly = true to render a
    // non-editable field.
    static bool text(
        const std::string& id,
        std::string& value,
        bool readOnly,
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

    static bool comboBox(
        const std::vector<std::string>& items,
        uint32_t &selected,
        const std::string& id,
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
        return Value::comboBox(label, items, selected, sameLine, fitPreview);
    }

    static bool comboBox(
        ItemListCallback itemsCb,
        uint32_t &selected,
        const std::string& id,
        bool sameLine = false,
        bool fitPreview = false
    ) {
        std::vector<std::string> items;
        itemsCb(items);
        return Value::comboBox(items, selected, id, sameLine, fitPreview);
    }
};

}
}
