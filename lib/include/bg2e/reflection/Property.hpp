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

#include <any>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace bg2e {
namespace reflection {

enum class PropertyType {
    Bool, Int, UInt, Float, Double, String,
    Vec2, Vec3, Vec4, Mat4,
    Color, Enum, Resource, Path,
    Object
};

enum class PropertyEditor {
    Default, Input, Slider, Drag,
    Checkbox, Combo, Color, Angle
};

struct PropertyMetadata {
    std::string displayName;
    std::string category;
    std::string tooltip;
    std::optional<double> min;
    std::optional<double> max;
    std::optional<double> step;
    // (label, value) pairs for Enum properties (future Combo editors).
    std::vector<std::pair<std::string, int64_t>> enumOptions;
};

// Maximum depth of Object-property chains a consumer may recurse into.
// A root reflected instance is depth 0; the targets of its object
// properties are depth 1, and so on. See TypeRegistry::objectChainDepth().
inline constexpr uint32_t maxObjectDepth = 3;

struct PropertyInfo {
    std::string name;
    PropertyType type = PropertyType::Float;
    PropertyEditor editor = PropertyEditor::Default;
    PropertyMetadata metadata;

    // Type-erased accessors. Instances are addressed as void*.
    std::function<std::any(const void*)> getter;
    std::function<void(void*, const std::any&)> setter;

    // Object properties (type == PropertyType::Object) only:
    // the sub-object is addressed by pointer and edited in place through
    // its own reflected setters; there is deliberately no parent setter.
    std::string objectTypeName;                             // TypeRegistry key of the sub-object type
    std::function<const void*(const void*)> objectGetter;   // address of the sub-object (always set)
    std::function<void*(void*)> objectMutableGetter;        // empty => sub-object is read-only

    // A property without a write path is read-only. There is no separate
    // flag: scalar properties are read-only without a setter; object
    // properties are read-only without a mutable object getter.
    bool isReadOnly() const
    {
        if (type == PropertyType::Object)
        {
            return !static_cast<bool>(objectMutableGetter);
        }
        return !static_cast<bool>(setter);
    }
};

}
}
