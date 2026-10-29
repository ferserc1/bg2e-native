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
#include <bg2e/ui/EditInteraction.hpp>
#include <bg2e/reflection/TypeInfo.hpp>

#include <any>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace bg2e {
namespace ui {

// Each segment identifies a property in a registered owner type. For an
// intermediate polymorphic segment, subtypeKey freezes the expected subtype;
// subtypeObject selects its derived view rather than the base view.
struct ReflectionPropertySegment {
    std::string ownerType;
    std::string property;
    std::string subtypeKey;
    bool subtypeObject = false;
};
using ReflectionPropertyPath = std::vector<ReflectionPropertySegment>;

struct ReflectionEditIntent {
    ReflectionPropertyPath path;
    reflection::PropertyType type = reflection::PropertyType::Float;
    std::any proposed; // Exact getter/setter value type, including Resource storage type.
};

struct ReflectionActionIntent {
    ReflectionPropertyPath objectPath;
    std::string ownerType;
    std::string action;
};

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

    // Optional typed mode. The session owns drafts, the application owns live
    // mutation. Bracket the whole editor frame, not each recursive call, with
    // session.beginFrame/endFrame. Cancellation is required before target rebind.
    // Arbitrary actions and polymorphic replacement have no implicit restoration
    // contract: actions require a separate adapter. Subtype replacement proposes
    // a key with PolymorphicObject type; the receiver must retain old/new objects.
    static bool drawEditIntents(void* instance, const reflection::TypeInfo& info,
        EditSession& session,
        const std::function<void(const ReflectionActionIntent&)>& action = {},
        const ReflectionPropertyPath& path = {});

    // Resolve afresh at execution/replay. Returned pointers are borrowed only for
    // that operation; never store them in a command. Null means stale/readonly.
    static const reflection::PropertyInfo* resolveProperty(void* root,
        const reflection::TypeInfo& rootInfo, const ReflectionPropertyPath& path,
        void*& owner);

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
