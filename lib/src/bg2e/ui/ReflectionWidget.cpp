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

#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Vector.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/ResourcePicker.hpp>

#include <any>
#include <charconv>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace bg2e {
namespace ui {

namespace {

using reflection::PropertyEditor;
using reflection::PropertyType;

// Visible label + unique ImGui ID. The property name is unique inside a
// TypeInfo, so it is enough to disambiguate widgets inside the same ID scope.
std::string labelFor(const reflection::PropertyInfo& prop)
{
    const auto& displayName = prop.metadata.displayName;
    return (displayName.empty() ? prop.name : displayName) + "##" + prop.name;
}

// Plain visible name, for widgets that show the text verbatim (Text)
std::string nameFor(const reflection::PropertyInfo& prop)
{
    return prop.metadata.displayName.empty() ? prop.name : prop.metadata.displayName;
}

float minOr(const reflection::PropertyMetadata& md, float fallback)
{
    return md.min ? static_cast<float>(*md.min) : fallback;
}

float maxOr(const reflection::PropertyMetadata& md, float fallback)
{
    return md.max ? static_cast<float>(*md.max) : fallback;
}

float stepOr(const reflection::PropertyMetadata& md, float fallback)
{
    return md.step ? static_cast<float>(*md.step) : fallback;
}

bool drawIntEditor(const std::string& label, int& v, const reflection::PropertyInfo& prop)
{
    const auto& md = prop.metadata;
    if (!md.min && !md.max)
    {
        return Numeric::drag(label, &v, stepOr(md, 1.0f));
    }
    int min = static_cast<int>(minOr(md, 0.0f));
    int max = static_cast<int>(maxOr(md, 100.0f));
    switch (prop.editor)
    {
    case PropertyEditor::Slider:
        return Numeric::sliderInt(label, &v, min, max);
    case PropertyEditor::Drag:
        return Numeric::drag(label, &v, stepOr(md, 1.0f), min, max);
    default:
        return Numeric::number(label, &v);
    }
}

bool drawFloatEditor(const std::string& label, float& v, const reflection::PropertyInfo& prop)
{
    const auto& md = prop.metadata;
    if (!md.min && !md.max)
    {
        return Numeric::drag(label, &v, stepOr(md, 0.1f));
    }
    float min = minOr(md, 0.0f);
    float max = maxOr(md, 1.0f);
    switch (prop.editor)
    {
    case PropertyEditor::Slider:
        return Numeric::sliderFloat(label, &v, min, max);
    case PropertyEditor::Drag:
        // Drag without an explicit range is left unclamped (0, 0)
        return Numeric::drag(
            label, &v, stepOr(md, 0.1f),
            md.min ? min : 0.0f, md.max ? max : 0.0f);
    case PropertyEditor::Angle:
        // Angles are edited in degrees; use the metadata range if present
        if (md.min && md.max)
        {
            return Numeric::sliderFloat(label, &v, min, max);
        }
        return Numeric::drag(label, &v, stepOr(md, 0.5f));
    default:
        return Numeric::number(label, &v);
    }
}

bool drawVec2Editor(const std::string& label, glm::vec2& v, const reflection::PropertyInfo& prop)
{
    const auto& md = prop.metadata;
    if (!md.min && !md.max)
    {
        return Vector::dragVec2(label, v, stepOr(md, 0.1f));
    }
    const float min = minOr(md, 0.0f);
    const float max = maxOr(md, 1.0f);
    switch (prop.editor)
    {
    case PropertyEditor::Slider:
        return Vector::sliderVec2(label, v, min, max);
    case PropertyEditor::Drag:
        return Vector::dragVec2(
            label, v, stepOr(md, 0.1f),
            md.min ? min : 0.0f, md.max ? max : 0.0f);
    case PropertyEditor::Angle:
        if (md.min && md.max)
        {
            return Vector::sliderVec2(label, v, min, max);
        }
        return Vector::dragVec2(label, v, stepOr(md, 0.5f));
    default:
        return Vector::vec2(label, v);
    }
}

bool drawVec3Editor(const std::string& label, glm::vec3& v, const reflection::PropertyInfo& prop)
{
    const auto& md = prop.metadata;
    if (!md.min && !md.max)
    {
        return Vector::dragVec3(label, v, stepOr(md, 0.1f));
    }
    const float min = minOr(md, 0.0f);
    const float max = maxOr(md, 1.0f);
    switch (prop.editor)
    {
    case PropertyEditor::Slider:
        return Vector::sliderVec3(label, v, min, max);
    case PropertyEditor::Drag:
        return Vector::dragVec3(
            label, v, stepOr(md, 0.1f),
            md.min ? min : 0.0f, md.max ? max : 0.0f);
    case PropertyEditor::Angle:
        if (md.min && md.max)
        {
            return Vector::sliderVec3(label, v, min, max);
        }
        return Vector::dragVec3(label, v, stepOr(md, 0.5f));
    default:
        return Vector::vec3(label, v);
    }
}

bool drawVec4Editor(const std::string& label, glm::vec4& v, const reflection::PropertyInfo& prop)
{
    const auto& md = prop.metadata;
    if (!md.min && !md.max)
    {
        return Vector::dragVec4(label, v, stepOr(md, 0.1f));
    }
    const float min = minOr(md, 0.0f);
    const float max = maxOr(md, 1.0f);
    switch (prop.editor)
    {
    case PropertyEditor::Slider:
        return Vector::sliderVec4(label, v, min, max);
    case PropertyEditor::Drag:
        return Vector::dragVec4(
            label, v, stepOr(md, 0.1f),
            md.min ? min : 0.0f, md.max ? max : 0.0f);
    case PropertyEditor::Angle:
        if (md.min && md.max)
        {
            return Vector::sliderVec4(label, v, min, max);
        }
        return Vector::dragVec4(label, v, stepOr(md, 0.5f));
    default:
        return Vector::vec4(label, v);
    }
}

} // anonymous namespace

bool ReflectionWidget::drawProperties(
    void* instance,
    const reflection::TypeInfo& info,
    uint32_t depth,
    const EditHandler& edit)
{
    // Group properties by category, preserving first-appearance order
    std::vector<std::string> categories;
    std::vector<std::vector<const reflection::PropertyInfo*>> groups;
    for (const auto& prop : info.properties)
    {
        size_t idx = 0;
        for (; idx < categories.size(); ++idx)
        {
            if (categories[idx] == prop.metadata.category)
            {
                break;
            }
        }
        if (idx == categories.size())
        {
            categories.push_back(prop.metadata.category);
            groups.emplace_back();
        }
        groups[idx].push_back(&prop);
    }

    bool changed = false;
    int id = 0;
    for (size_t g = 0; g < groups.size(); ++g)
    {
        bool useTree = !categories[g].empty();
        bool open = true;
        if (useTree)
        {
            open = Group::beginTree(
                categories[g] + "##" + info.typeName + std::to_string(depth) + "_" + std::to_string(g));
        }
        if (open)
        {
            for (auto* prop : groups[g])
            {
                Group::pushId(id++);
                if (drawProperty(instance, *prop, depth, edit))
                {
                    changed = true;
                }
                Group::popId();
            }
        }
        if (useTree && open)
        {
            Group::endTree();
        }
    }
    return changed;
}

const reflection::PropertyInfo* ReflectionWidget::resolveProperty(void* root,
    const reflection::TypeInfo& rootInfo, const ReflectionPropertyPath& path, void*& owner)
{
    owner = nullptr;
    if (!root || path.empty() || path.size() > reflection::maxObjectDepth + 1)
    {
        return nullptr;
    }
    auto* current = root;
    const auto* info = &rootInfo;
    auto& registry = reflection::TypeRegistry::get();
    for (size_t i = 0; i < path.size(); ++i)
    {
        const auto& segment = path[i];
        if (!info || info->typeName != segment.ownerType)
        {
            return nullptr;
        }
        const auto* property = info->property(segment.property);
        if (!property || property->isReadOnly())
        {
            return nullptr;
        }
        if (i + 1 == path.size())
        {
            owner = current;
            return property;
        }
        if (property->type == PropertyType::Object && property->objectMutableGetter)
        {
            current = property->objectMutableGetter(current);
            info = registry.type(property->objectTypeName);
        }
        else if (property->type == PropertyType::PolymorphicObject &&
            property->polymorphicObjectMutableGetter && property->polymorphicTypeKey &&
            property->polymorphicTypeKey(current) == segment.subtypeKey)
        {
            current = property->polymorphicObjectMutableGetter(current);
            info = registry.type(property->polymorphicBaseTypeName);
            if (current && segment.subtypeObject)
            {
                const auto* subtype = registry.subtype(property->polymorphicBaseTypeName, segment.subtypeKey);
                current = registry.subtypeObject(property->polymorphicBaseTypeName, segment.subtypeKey, current);
                info = subtype ? registry.type(subtype->typeName) : nullptr;
            }
        }
        else
        {
            return nullptr;
        }
        if (!current)
        {
            return nullptr;
        }
    }
    return nullptr;
}


bool ReflectionWidget::drawEditIntents(void* instance, const reflection::TypeInfo& info,
    EditSession& session, const std::function<void(const ReflectionActionIntent&)>& action,
    const ReflectionPropertyPath& path)
{
    if (!instance || path.size() > reflection::maxObjectDepth)
    {
        return false;
    }
    bool changed = false;
    auto& registry = reflection::TypeRegistry::get();
    for (const auto& property : info.properties)
    {
        auto next = path;
        next.push_back({info.typeName, property.name, {}, false});
        // Length-prefix every component: names containing separators cannot alias.
        std::string key;
        for (const auto& segment : next)
        {
            for (const auto* part : {&segment.ownerType, &segment.property, &segment.subtypeKey})
            {
                key += std::to_string(part->size()) + ":" + *part;
            }
            key += segment.subtypeObject ? "1" : "0";
        }
        Group::pushId(key);
        if (property.type == PropertyType::Object || property.type == PropertyType::PolymorphicObject)
        {
            if (Group::beginTree(nameFor(property)))
            {
                if (!property.isReadOnly() && next.size() <= reflection::maxObjectDepth)
                {
                    if (property.type == PropertyType::Object)
                    {
                        auto* child = property.objectMutableGetter ? property.objectMutableGetter(instance) : nullptr;
                        const auto* childInfo = registry.type(property.objectTypeName);
                        if (child && childInfo)
                        {
                            changed = drawEditIntents(child, *childInfo, session, action, next) || changed;
                        }
                    }
                    else
                    {
                        auto* child = property.polymorphicObjectMutableGetter
                            ? property.polymorphicObjectMutableGetter(instance) : nullptr;
                        auto subtypeKey = property.polymorphicTypeKey ? property.polymorphicTypeKey(instance) : std::string{};
                        const auto* subtype = registry.subtype(property.polymorphicBaseTypeName, subtypeKey);
                        std::vector<std::string> labels, keys;
                        uint32_t selected = 0;
                        for (const auto& option : registry.subtypes(property.polymorphicBaseTypeName))
                        {
                            if (!property.polymorphicSubtypeKeys.empty() &&
                                std::find(property.polymorphicSubtypeKeys.begin(), property.polymorphicSubtypeKeys.end(), option.key) == property.polymorphicSubtypeKeys.end())
                            {
                                continue;
                            }
                            if (option.key == subtypeKey)
                            {
                                selected = static_cast<uint32_t>(keys.size());
                            }
                            labels.push_back(option.displayName);
                            keys.push_back(option.key);
                        }
                        if (property.polymorphicObjectReplacer && !keys.empty())
                        {
                            if (Value::comboBox("Type", labels, selected) && keys[selected] != subtypeKey)
                            {
                                ReflectionEditIntent replacement{next, property.type, keys[selected]};
                                changed = session.submit(key, std::move(replacement), true, {}) || changed;
                            }
                        }
                        else
                        {
                            Text::text("Type: " + subtypeKey);
                        }
                        if (child && subtype)
                        {
                            next.back().subtypeKey = subtypeKey;
                            const auto* baseInfo = registry.type(property.polymorphicBaseTypeName);
                            if (baseInfo && subtype->typeName != property.polymorphicBaseTypeName)
                            {
                                changed = drawEditIntents(child, *baseInfo, session, action, next) || changed;
                            }
                            auto* derived = registry.subtypeObject(property.polymorphicBaseTypeName, subtypeKey, child);
                            const auto* derivedInfo = registry.type(subtype->typeName);
                            if (derived && derivedInfo)
                            {
                                next.back().subtypeObject = true;
                                changed = drawEditIntents(derived, *derivedInfo, session, action, next) || changed;
                            }
                        }
                    }
                }
                Group::endTree();
            }
        }
        else if (property.getter)
        {
            ReflectionEditIntent intent{next, property.type, property.getter(instance)};
            auto draft = session.value(key, intent);
            intent = std::any_cast<ReflectionEditIntent>(std::move(draft));
            bool edited = false;
            const auto state = EditInteraction::draw([&]
            {
                edited = editValue(intent.proposed, property);
            });
            changed = session.submit(key, std::move(intent), edited, state) || changed;
        }
        Group::popId();
    }
    for (const auto& entry : info.actions)
    {
        Group::beginDisabled(!action);
        if (Button::button((entry.displayName.empty() ? entry.name : entry.displayName) + "##" + entry.name) && action)
        {
            // The application must supply a reversible adapter or explicitly
            // execute this as a nonhistorical action. No callable is captured.
            action({path, info.typeName, entry.name});
        }
        Group::endDisabled();
    }
    return changed;
}


bool ReflectionWidget::editValue(std::any& value, const reflection::PropertyInfo& property, bool editable)
{
    auto proxy = property;
    // Replace both access paths: delayed settings must never invoke target setters.
    proxy.getter = [&value](const void*)
    {
        return value;
    };
    proxy.setter = [&value](void*, const std::any& next)
    {
        value = next;
    };
    const bool writable = editable && !property.isReadOnly();
    Group::beginDisabled(!writable);
    struct DisabledScope {
        ~DisabledScope()
        {
            Group::endDisabled();
        }
    } disabledScope;
    bool changed = false;
    const auto label = labelFor(proxy);
    if (proxy.type == PropertyType::UInt)
    {
        auto text = std::to_string(std::any_cast<uint32_t>(value));
        if (!proxy.metadata.min && !proxy.metadata.max)
        {
            auto next = std::any_cast<uint32_t>(value);
            changed = Numeric::drag(label, &next, stepOr(proxy.metadata, 1.0f));
            if (changed)
            {
                value = next;
            }
        }
        else if (Value::text(label, text, 32))
        {
            uint32_t next = 0;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), next);
            if (result.ec == std::errc{} && result.ptr == text.data() + text.size())
            {
                value = next;
                changed = true;
            }
        }
    }
    else if (proxy.type == PropertyType::Double)
    {
        auto next = std::any_cast<double>(value);
        changed = !proxy.metadata.min && !proxy.metadata.max
            ? Numeric::drag(label, &next, stepOr(proxy.metadata, 0.1f))
            : Numeric::number(label, &next);
        if (changed)
        {
            value = next;
        }
    }
    else if (proxy.type == PropertyType::String || proxy.type == PropertyType::Path)
    {
        auto text = proxy.type == PropertyType::Path ? std::any_cast<std::filesystem::path>(value).string() : std::any_cast<std::string>(value);
        const auto capacity = std::max(size_t(1024), text.size() + 256);
        if (capacity > size_t(std::numeric_limits<int>::max()))
        {
            Text::text("Value is too long to edit");
        }
        else if (Value::text(label, text, int(capacity)))
        {
            value = proxy.type == PropertyType::Path ? std::any(std::filesystem::path(text)) : std::any(text);
            changed = true;
        }
    }
    else if (proxy.type == PropertyType::Object || proxy.type == PropertyType::PolymorphicObject)
    {
        Text::text(nameFor(proxy) + ": unsupported settings type");
    }
    else
    {
        changed = drawScalarProperty(nullptr, proxy);
    }
    Text::tooltip(proxy.metadata.tooltip);
    return writable && changed;
}

void ReflectionWidget::drawActions(
    void* instance,
    const reflection::TypeInfo& info,
    const EditHandler& edit)
{
    int id = 0;
    for (const auto& action : info.actions)
    {
        Group::pushId(id++);
        const auto& label = action.displayName.empty() ? action.name : action.displayName;
        if (Button::button(label + "##" + action.name))
        {
            if (edit)
            {
                edit(action.invoke, true);
            }
            else
            {
                action.invoke(instance);
            }
        }
        Text::tooltip(action.tooltip);
        Group::popId();
    }
}

bool ReflectionWidget::drawProperty(
    void* instance,
    const reflection::PropertyInfo& prop,
    uint32_t depth,
    const EditHandler& edit)
{
    if (prop.type == PropertyType::Object)
    {
        return drawObjectProperty(instance, prop, depth, edit);
    }
    if (prop.type == PropertyType::PolymorphicObject)
    {
        return drawPolymorphicObjectProperty(instance, prop, depth, edit);
    }
    if (edit && prop.getter)
    {
        auto value = prop.getter(instance);
        if (!editValue(value, prop))
        {
            return false;
        }
        edit([setter = prop.setter, value = std::move(value)](void* target)
        {
            setter(target, value);
        }, prop.type == PropertyType::Resource || prop.type == PropertyType::Path);
        return true;
    }
    return drawScalarProperty(instance, prop);
}

bool ReflectionWidget::drawScalarProperty(
    void* instance,
    const reflection::PropertyInfo& prop)
{
    const auto label = labelFor(prop);
    const bool readOnly = prop.isReadOnly();
    bool changed = false;

    if (readOnly)
    {
        Group::beginDisabled();
    }

    switch (prop.type)
    {
    case PropertyType::Bool: {
        auto v = std::any_cast<bool>(prop.getter(instance));
        changed = Button::checkBox(label, &v);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Int: {
        int v = static_cast<int>(std::any_cast<int32_t>(prop.getter(instance)));
        changed = drawIntEditor(label, v, prop);
        if (changed)
        {
            prop.setter(instance, static_cast<int32_t>(v));
        }
        break;
    }
    case PropertyType::UInt: {
        if (!prop.metadata.min && !prop.metadata.max)
        {
            auto v = std::any_cast<uint32_t>(prop.getter(instance));
            changed = Numeric::drag(label, &v, stepOr(prop.metadata, 1.0f));
            if (changed)
            {
                prop.setter(instance, v);
            }
            break;
        }
        int v = static_cast<int>(std::any_cast<uint32_t>(prop.getter(instance)));
        changed = drawIntEditor(label, v, prop);
        if (changed && v >= 0)
        {
            prop.setter(instance, static_cast<uint32_t>(v));
        }
        break;
    }
    case PropertyType::Float: {
        auto v = std::any_cast<float>(prop.getter(instance));
        changed = drawFloatEditor(label, v, prop);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Double: {
        auto v = std::any_cast<double>(prop.getter(instance));
        if (!prop.metadata.min && !prop.metadata.max)
        {
            changed = Numeric::drag(label, &v, stepOr(prop.metadata, 0.1f));
        }
        else if (prop.editor == PropertyEditor::Default || prop.editor == PropertyEditor::Input)
        {
            changed = Numeric::number(label, &v);
        }
        else
        {
            // Slider/drag editors work on a float temporary
            float f = static_cast<float>(v);
            changed = drawFloatEditor(label, f, prop);
            v = static_cast<double>(f);
        }
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::String: {
        auto v = std::any_cast<std::string>(prop.getter(instance));
        changed = Value::text(label, v);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Vec2: {
        auto v = std::any_cast<glm::vec2>(prop.getter(instance));
        changed = drawVec2Editor(label, v, prop);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Vec3: {
        auto v = std::any_cast<glm::vec3>(prop.getter(instance));
        changed = drawVec3Editor(label, v, prop);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Vec4: {
        auto v = std::any_cast<glm::vec4>(prop.getter(instance));
        changed = drawVec4Editor(label, v, prop);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Mat4: {
        auto v = std::any_cast<glm::mat4>(prop.getter(instance));
        changed = Vector::mat4(label, v);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Color: {
        auto v = std::any_cast<base::Color>(prop.getter(instance));
        changed = Value::colorPicker(label, v);
        if (changed)
        {
            prop.setter(instance, v);
        }
        break;
    }
    case PropertyType::Enum: {
        const auto currentValue = std::any_cast<int64_t>(prop.getter(instance));
        if (prop.metadata.enumOptions.empty())
        {
            Group::beginDisabled();
            Text::text(nameFor(prop) + ": <no enum options>");
            Group::endDisabled();
            break;
        }

        std::vector<std::string> labels;
        std::vector<int64_t> values;
        uint32_t selected = 0;
        bool currentFound = false;
        for (const auto& [optionLabel, optionValue] : prop.metadata.enumOptions)
        {
            if (optionValue == currentValue)
            {
                selected = static_cast<uint32_t>(labels.size());
                currentFound = true;
            }
            labels.push_back(optionLabel);
            values.push_back(optionValue);
        }
        if (!currentFound)
        {
            selected = static_cast<uint32_t>(labels.size());
            labels.push_back("Unknown (" + std::to_string(currentValue) + ")");
            values.push_back(currentValue);
        }
        changed = Value::comboBox(label, labels, selected);
        if (changed)
        {
            prop.setter(instance, values[selected]);
        }
        break;
    }
    case PropertyType::Path: {
        auto v = std::any_cast<std::filesystem::path>(prop.getter(instance));
        Text::text(nameFor(prop) + ": " + v.string());
        break;
    }
    case PropertyType::Resource: {
        std::filesystem::path v;
        if (prop.metadata.resourceValueType == PropertyType::String)
        {
            v = std::any_cast<std::string>(prop.getter(instance));
        }
        else
        {
            v = std::any_cast<std::filesystem::path>(prop.getter(instance));
        }
        changed = ResourcePicker::draw(label, v, prop.metadata, readOnly);
        if (changed)
        {
            if (prop.metadata.resourceValueType == PropertyType::String)
            {
                prop.setter(instance, v.string());
            }
            else
            {
                prop.setter(instance, v);
            }
        }
        break;
    }
    default:
        Text::text(nameFor(prop) + ": <not supported>");
        break;
    }

    if (readOnly)
    {
        Group::endDisabled();
    }

    Text::tooltip(prop.metadata.tooltip);
    return changed;
}

bool ReflectionWidget::drawObjectProperty(
    void* instance,
    const reflection::PropertyInfo& prop,
    uint32_t depth,
    const EditHandler& edit)
{
    const auto& displayName = prop.metadata.displayName;
    const auto label = displayName.empty() ? prop.name : displayName;

    const auto* objectInfo = reflection::TypeRegistry::get().type(prop.objectTypeName);
    const void* subObject = prop.objectGetter ? prop.objectGetter(instance) : nullptr;

    if (!objectInfo || !subObject)
    {
        // Sub-object type not registered, or no instance: fallback label
        Group::beginDisabled();
        Text::text(label + ": <not registered>");
        Group::endDisabled();
        Text::tooltip(prop.metadata.tooltip);
        return false;
    }

    if (depth >= reflection::maxObjectDepth)
    {
        Text::text(label + ": <max depth reached>");
        Text::tooltip(prop.metadata.tooltip);
        return false;
    }

    bool changed = false;
    if (Group::beginTree(label + "##" + prop.name))
    {
        const bool readOnly = prop.isReadOnly();
        if (readOnly)
        {
            Group::beginDisabled();
        }

        void* mutableSubObject = readOnly ? nullptr : prop.objectMutableGetter(instance);
        EditHandler nestedEdit;
        if (edit)
        {
            nestedEdit = [edit, resolve = prop.objectMutableGetter](auto operation, bool resources)
            {
                edit([resolve, operation = std::move(operation)](void* parent)
                {
                    if (auto* child = resolve ? resolve(parent) : nullptr)
                    {
                        operation(child);
                    }
                }, resources);
            };
        }
        changed = drawProperties(
                      mutableSubObject ? mutableSubObject : const_cast<void*>(subObject),
                      *objectInfo,
                      depth + 1, nestedEdit) &&
                  !readOnly;
        drawActions(mutableSubObject ? mutableSubObject : const_cast<void*>(subObject), *objectInfo, nestedEdit);

        if (readOnly)
        {
            Group::endDisabled();
        }
        Group::endTree();
    }
    Text::tooltip(prop.metadata.tooltip);
    return changed;
}

bool ReflectionWidget::drawPolymorphicObjectProperty(
    void* instance,
    const reflection::PropertyInfo& prop,
    uint32_t depth,
    const EditHandler& edit)
{
    const auto label = nameFor(prop);
    auto& registry = reflection::TypeRegistry::get();
    std::vector<reflection::SubtypeInfo> options;
    if (prop.polymorphicSubtypeKeys.empty())
    {
        options = registry.subtypes(prop.polymorphicBaseTypeName);
    }
    else
    {
        options.reserve(prop.polymorphicSubtypeKeys.size());
        for (const auto& key : prop.polymorphicSubtypeKeys)
        {
            if (const auto* option = registry.subtype(prop.polymorphicBaseTypeName, key))
            {
                options.push_back(*option);
            }
        }
    }
    const void* currentObject = prop.polymorphicObjectGetter
                                    ? prop.polymorphicObjectGetter(instance)
                                    : nullptr;
    std::string currentKey = currentObject && prop.polymorphicTypeKey
                                 ? prop.polymorphicTypeKey(instance)
                                 : std::string{};

    if (currentObject && currentKey.empty())
    {
        Group::beginDisabled();
        Text::text(label + ": <unknown subtype>");
        Group::endDisabled();
        Text::tooltip(prop.metadata.tooltip);
        return false;
    }

    const reflection::SubtypeInfo* activeSubtype = currentKey.empty()
                                                       ? nullptr
                                                       : registry.subtype(prop.polymorphicBaseTypeName, currentKey);
    if (currentObject && !activeSubtype)
    {
        Group::beginDisabled();
        Text::text(label + ": <subtype not registered>");
        Group::endDisabled();
        Text::tooltip(prop.metadata.tooltip);
        return false;
    }

    bool changed = false;
    if (Group::beginTree(label + "##" + prop.name))
    {
        std::vector<std::string> subtypeLabels;
        std::vector<std::string> subtypeKeys;
        uint32_t selected = 0;
        for (const auto& option : options)
        {
            if (option.key == currentKey)
            {
                selected = static_cast<uint32_t>(subtypeLabels.size());
            }
            subtypeLabels.push_back(option.displayName);
            subtypeKeys.push_back(option.key);
        }

        if (!currentObject)
        {
            selected = static_cast<uint32_t>(subtypeLabels.size());
            subtypeLabels.push_back("None");
            subtypeKeys.emplace_back();
        }

        const bool canReplace = static_cast<bool>(prop.polymorphicObjectReplacer);
        if (options.empty() || (!currentObject && !canReplace))
        {
            Group::beginDisabled();
            Text::text("Type: <none available>");
            Group::endDisabled();
        }
        else
        {
            if (!canReplace)
            {
                Group::beginDisabled();
            }
            const bool selectionChanged = Value::comboBox(
                "Type##" + prop.name,
                subtypeLabels,
                selected);
            if (!canReplace)
            {
                Group::endDisabled();
            }

            if (selectionChanged && canReplace && !subtypeKeys[selected].empty())
            {
                if (edit)
                {
                    edit([replace = prop.polymorphicObjectReplacer, key = subtypeKeys[selected]](void* parent)
                    {
                        if (!replace(parent, key))
                        {
                            throw std::runtime_error("Reflected object replacement failed");
                        }
                    }, true);
                    // Keep child controls on the next frame's actual subtype.
                    Group::endTree();
                    Text::tooltip(prop.metadata.tooltip);
                    return true;
                }
                changed = prop.polymorphicObjectReplacer(instance, subtypeKeys[selected]);

                // Replacement invalidates every pointer obtained above.
                currentObject = prop.polymorphicObjectGetter
                                    ? prop.polymorphicObjectGetter(instance)
                                    : nullptr;
                currentKey = currentObject && prop.polymorphicTypeKey
                                 ? prop.polymorphicTypeKey(instance)
                                 : std::string{};
                activeSubtype = currentKey.empty()
                                    ? nullptr
                                    : registry.subtype(prop.polymorphicBaseTypeName, currentKey);
            }
        }

        if (currentObject && activeSubtype)
        {
            const auto* baseInfo = registry.type(prop.polymorphicBaseTypeName);
            const auto* subtypeInfo = registry.type(activeSubtype->typeName);
            if (!subtypeInfo)
            {
                Group::beginDisabled();
                Text::text("<subtype reflection not registered>");
                Group::endDisabled();
            }
            else if (depth >= reflection::maxObjectDepth)
            {
                Text::text("<max depth reached>");
            }
            else
            {
                void* mutableBaseObject = prop.polymorphicObjectMutableGetter
                                              ? prop.polymorphicObjectMutableGetter(instance)
                                              : nullptr;
                void* mutableSubtypeObject = registry.subtypeObject(
                    prop.polymorphicBaseTypeName,
                    currentKey,
                    mutableBaseObject);
                const void* subtypeObject = registry.subtypeObject(
                    prop.polymorphicBaseTypeName,
                    currentKey,
                    currentObject);
                const bool readOnly = mutableBaseObject == nullptr;
                if (readOnly)
                {
                    Group::beginDisabled();
                }

                auto* editBaseObject = mutableBaseObject
                                           ? mutableBaseObject
                                           : const_cast<void*>(currentObject);
                const auto nestedHandler = [&](bool subtype) -> EditHandler
                {
                    if (!edit)
                    {
                        return {};
                    }
                    return [edit, prop, currentKey, subtype](auto operation, bool resources)
                    {
                        edit([prop, currentKey, subtype, operation = std::move(operation)](void* parent)
                        {
                            if (!prop.polymorphicTypeKey || prop.polymorphicTypeKey(parent) != currentKey)
                            {
                                return;
                            }
                            auto* child = prop.polymorphicObjectMutableGetter
                                ? prop.polymorphicObjectMutableGetter(parent) : nullptr;
                            if (child && subtype)
                            {
                                child = reflection::TypeRegistry::get().subtypeObject(
                                    prop.polymorphicBaseTypeName, currentKey, child);
                            }
                            if (child)
                            {
                                operation(child);
                            }
                        }, resources);
                    };
                };
                if (baseInfo && activeSubtype->typeName != prop.polymorphicBaseTypeName)
                {
                    changed = drawProperties(editBaseObject, *baseInfo, depth + 1, nestedHandler(false)) || changed;
                    drawActions(editBaseObject, *baseInfo, nestedHandler(false));
                }

                auto* editSubtypeObject = mutableSubtypeObject
                                              ? mutableSubtypeObject
                                              : const_cast<void*>(subtypeObject);
                if (editSubtypeObject)
                {
                    changed = drawProperties(editSubtypeObject, *subtypeInfo, depth + 1, nestedHandler(true)) || changed;
                    drawActions(editSubtypeObject, *subtypeInfo, nestedHandler(true));
                }
                else
                {
                    Text::text("<invalid subtype object>");
                }
                if (readOnly)
                {
                    Group::endDisabled();
                }
            }
        }
        else if (!currentObject)
        {
            Group::beginDisabled();
            Text::text("<no object>");
            Group::endDisabled();
        }

        Group::endTree();
    }
    Text::tooltip(prop.metadata.tooltip);
    return changed;
}

}
}
