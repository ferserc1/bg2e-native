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
std::string labelFor(const reflection::PropertyInfo & prop)
{
    const auto & displayName = prop.metadata.displayName;
    return (displayName.empty() ? prop.name : displayName) + "##" + prop.name;
}

// Plain visible name, for widgets that show the text verbatim (Text)
std::string nameFor(const reflection::PropertyInfo & prop)
{
    return prop.metadata.displayName.empty() ? prop.name : prop.metadata.displayName;
}

float minOr(const reflection::PropertyMetadata & md, float fallback)
{
    return md.min ? static_cast<float>(*md.min) : fallback;
}

float maxOr(const reflection::PropertyMetadata & md, float fallback)
{
    return md.max ? static_cast<float>(*md.max) : fallback;
}

float stepOr(const reflection::PropertyMetadata & md, float fallback)
{
    return md.step ? static_cast<float>(*md.step) : fallback;
}

bool drawIntEditor(const std::string & label, int & v, const reflection::PropertyInfo & prop)
{
    const auto & md = prop.metadata;
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

bool drawFloatEditor(const std::string & label, float & v, const reflection::PropertyInfo & prop)
{
    const auto & md = prop.metadata;
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
            md.min ? min : 0.0f, md.max ? max : 0.0f
        );
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

} // anonymous namespace

bool ReflectionWidget::drawProperties(
    void * instance,
    const reflection::TypeInfo & info,
    uint32_t depth
) {
    // Group properties by category, preserving first-appearance order
    std::vector<std::string> categories;
    std::vector<std::vector<const reflection::PropertyInfo*>> groups;
    for (const auto & prop : info.properties)
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
                categories[g] + "##" + info.typeName + std::to_string(depth) + "_" + std::to_string(g)
            );
        }
        if (open)
        {
            for (auto * prop : groups[g])
            {
                Group::pushId(id++);
                if (drawProperty(instance, *prop, depth))
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

void ReflectionWidget::drawActions(
    void * instance,
    const reflection::TypeInfo & info
) {
    int id = 0;
    for (const auto & action : info.actions)
    {
        Group::pushId(id++);
        const auto & label = action.displayName.empty() ? action.name : action.displayName;
        if (Button::button(label + "##" + action.name))
        {
            action.invoke(instance);
        }
        Text::tooltip(action.tooltip);
        Group::popId();
    }
}

bool ReflectionWidget::drawProperty(
    void * instance,
    const reflection::PropertyInfo & prop,
    uint32_t depth
) {
    if (prop.type == PropertyType::Object)
    {
        return drawObjectProperty(instance, prop, depth);
    }
    if (prop.type == PropertyType::PolymorphicObject)
    {
        return drawPolymorphicObjectProperty(instance, prop, depth);
    }
    return drawScalarProperty(instance, prop);
}

bool ReflectionWidget::drawScalarProperty(
    void * instance,
    const reflection::PropertyInfo & prop
) {
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
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Int: {
        int v = static_cast<int>(std::any_cast<int32_t>(prop.getter(instance)));
        changed = drawIntEditor(label, v, prop);
        if (changed) prop.setter(instance, static_cast<int32_t>(v));
        break;
    }
    case PropertyType::UInt: {
        int v = static_cast<int>(std::any_cast<uint32_t>(prop.getter(instance)));
        changed = drawIntEditor(label, v, prop);
        if (changed && v >= 0) prop.setter(instance, static_cast<uint32_t>(v));
        break;
    }
    case PropertyType::Float: {
        auto v = std::any_cast<float>(prop.getter(instance));
        changed = drawFloatEditor(label, v, prop);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Double: {
        auto v = std::any_cast<double>(prop.getter(instance));
        if (prop.editor == PropertyEditor::Default || prop.editor == PropertyEditor::Input)
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
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::String: {
        auto v = std::any_cast<std::string>(prop.getter(instance));
        changed = Value::text(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec2: {
        auto v = std::any_cast<glm::vec2>(prop.getter(instance));
        changed = Vector::vec2(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec3: {
        auto v = std::any_cast<glm::vec3>(prop.getter(instance));
        changed = Vector::vec3(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec4: {
        auto v = std::any_cast<glm::vec4>(prop.getter(instance));
        changed = Vector::vec4(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Mat4: {
        auto v = std::any_cast<glm::mat4>(prop.getter(instance));
        changed = Vector::mat4(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Color: {
        auto v = std::any_cast<base::Color>(prop.getter(instance));
        changed = Value::colorPicker(label, v);
        if (changed) prop.setter(instance, v);
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
        for (const auto & [optionLabel, optionValue] : prop.metadata.enumOptions)
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
        if (changed) prop.setter(instance, values[selected]);
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
    void * instance,
    const reflection::PropertyInfo & prop,
    uint32_t depth
) {
    const auto & displayName = prop.metadata.displayName;
    const auto label = displayName.empty() ? prop.name : displayName;

    const auto * objectInfo = reflection::TypeRegistry::get().type(prop.objectTypeName);
    const void * subObject = prop.objectGetter ? prop.objectGetter(instance) : nullptr;

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

        void * mutableSubObject = readOnly ? nullptr : prop.objectMutableGetter(instance);
        changed = drawProperties(
            mutableSubObject ? mutableSubObject : const_cast<void*>(subObject),
            *objectInfo,
            depth + 1
        ) && !readOnly;
        drawActions(mutableSubObject ? mutableSubObject : const_cast<void*>(subObject), *objectInfo);

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
    void * instance,
    const reflection::PropertyInfo & prop,
    uint32_t depth
) {
    const auto label = nameFor(prop);
    auto & registry = reflection::TypeRegistry::get();
    std::vector<reflection::SubtypeInfo> options;
    if (prop.polymorphicSubtypeKeys.empty())
    {
        options = registry.subtypes(prop.polymorphicBaseTypeName);
    }
    else
    {
        options.reserve(prop.polymorphicSubtypeKeys.size());
        for (const auto & key : prop.polymorphicSubtypeKeys)
        {
            if (const auto * option = registry.subtype(prop.polymorphicBaseTypeName, key))
            {
                options.push_back(*option);
            }
        }
    }
    const void * currentObject = prop.polymorphicObjectGetter
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

    const reflection::SubtypeInfo * activeSubtype = currentKey.empty()
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
        for (const auto & option : options)
        {
            if (option.key == currentKey) selected = static_cast<uint32_t>(subtypeLabels.size());
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
            if (!canReplace) Group::beginDisabled();
            const bool selectionChanged = Value::comboBox(
                "Type##" + prop.name,
                subtypeLabels,
                selected
            );
            if (!canReplace) Group::endDisabled();

            if (selectionChanged && canReplace && !subtypeKeys[selected].empty())
            {
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
            const auto * baseInfo = registry.type(prop.polymorphicBaseTypeName);
            const auto * subtypeInfo = registry.type(activeSubtype->typeName);
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
                void * mutableBaseObject = prop.polymorphicObjectMutableGetter
                    ? prop.polymorphicObjectMutableGetter(instance)
                    : nullptr;
                void * mutableSubtypeObject = registry.subtypeObject(
                    prop.polymorphicBaseTypeName,
                    currentKey,
                    mutableBaseObject
                );
                const void * subtypeObject = registry.subtypeObject(
                    prop.polymorphicBaseTypeName,
                    currentKey,
                    currentObject
                );
                const bool readOnly = mutableBaseObject == nullptr;
                if (readOnly) Group::beginDisabled();

                auto * editBaseObject = mutableBaseObject
                    ? mutableBaseObject
                    : const_cast<void*>(currentObject);
                if (baseInfo && activeSubtype->typeName != prop.polymorphicBaseTypeName)
                {
                    changed = drawProperties(editBaseObject, *baseInfo, depth + 1) || changed;
                    drawActions(editBaseObject, *baseInfo);
                }

                auto * editSubtypeObject = mutableSubtypeObject
                    ? mutableSubtypeObject
                    : const_cast<void*>(subtypeObject);
                if (editSubtypeObject)
                {
                    changed = drawProperties(editSubtypeObject, *subtypeInfo, depth + 1) || changed;
                    drawActions(editSubtypeObject, *subtypeInfo);
                }
                else
                {
                    Text::text("<invalid subtype object>");
                }
                if (readOnly) Group::endDisabled();
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
