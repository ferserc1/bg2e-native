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
#include <bg2e/reflection/TypeInfo.hpp>

#include <cstdint>

namespace bg2e {
namespace ui {

// Generic ImGui drawer for bg2e::reflection metadata. It builds a user
// interface for any reflected type, addressing instances as void*, using
// only the bg2e::ui wrapper classes (Text, Group, Button, Numeric, Vector, Value).
class BG2E_API ReflectionWidget {
public:
    // Draws an editor for every property described by 'info', grouped by
    // category (first-appearance order). 'instance' addresses the reflected
    // object. 'depth' is the Object-property nesting level (a root instance
    // is depth 0, capped at reflection::maxObjectDepth).
    // Returns true if any property value was modified.
    static bool drawProperties(
        void * instance,
        const reflection::TypeInfo & info,
        uint32_t depth = 0
    );

    // Draws one button per action described by 'info'.
    static void drawActions(
        void * instance,
        const reflection::TypeInfo & info
    );

protected:
    static bool drawProperty(
        void * instance,
        const reflection::PropertyInfo & prop,
        uint32_t depth
    );

    static bool drawScalarProperty(
        void * instance,
        const reflection::PropertyInfo & prop
    );

    static bool drawObjectProperty(
        void * instance,
        const reflection::PropertyInfo & prop,
        uint32_t depth
    );
};

}
}
