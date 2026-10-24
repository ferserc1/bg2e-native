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
#include <functional>

namespace bg2e {
namespace ui {

// Generic ImGui drawer for bg2e::reflection metadata. It builds a user
// interface for any reflected type, addressing instances as void*, using
// only the bg2e::ui wrapper classes (Text, Group, Button, Numeric, Vector, Value).
class BG2E_API ReflectionWidget {
  public:
    // Receives an operation on the root instance and whether it can replace
    // resources. The handler queues it against a live, independently checked
    // target. No live setters or actions run while drawing with this handler.
    using EditHandler = std::function<void(std::function<void(void*)>, bool)>;

    // Draws an editor for every property described by 'info', grouped by
    // category (first-appearance order). 'instance' addresses the reflected
    // object. 'depth' is the Object-property nesting level (a root instance
    // is depth 0, capped at reflection::maxObjectDepth).
    // Returns true if any property value was modified or queued for editing.
    // With a handler, nested object paths are resolved again when the operation
    // executes; operations never retain borrowed pointers to child objects.
    static bool drawProperties(
        void* instance,
        const reflection::TypeInfo& info,
        uint32_t depth = 0,
        const EditHandler& edit = {});

    // Edits a transient scalar value only; never invokes the original property's
    // accessors. The caller validates/queues the result against its live target.
    static bool editValue(std::any& value, const reflection::PropertyInfo& property, bool editable = true);

    // Draws one button per action described by 'info'.
    static void drawActions(
        void* instance,
        const reflection::TypeInfo& info,
        const EditHandler& edit = {});

  protected:
    static bool drawProperty(
        void* instance,
        const reflection::PropertyInfo& prop,
        uint32_t depth,
        const EditHandler& edit = {});

    static bool drawScalarProperty(
        void* instance,
        const reflection::PropertyInfo& prop);

    static bool drawObjectProperty(
        void* instance,
        const reflection::PropertyInfo& prop,
        uint32_t depth,
        const EditHandler& edit = {});

    static bool drawPolymorphicObjectProperty(
        void* instance,
        const reflection::PropertyInfo& prop,
        uint32_t depth,
        const EditHandler& edit = {});
};

}
}
