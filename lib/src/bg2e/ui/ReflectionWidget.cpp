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
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>

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
        return Input::sliderInt(label, &v, min, max);
    case PropertyEditor::Drag:
        return Input::drag(label, &v, stepOr(md, 1.0f), min, max);
    default:
        return Input::number(label, &v);
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
        return Input::sliderFloat(label, &v, min, max);
    case PropertyEditor::Drag:
        // Drag without an explicit range is left unclamped (0, 0)
        return Input::drag(
            label, &v, stepOr(md, 0.1f),
            md.min ? min : 0.0f, md.max ? max : 0.0f
        );
    case PropertyEditor::Angle:
        // Angles are edited in degrees; use the metadata range if present
        if (md.min && md.max)
        {
            return Input::sliderFloat(label, &v, min, max);
        }
        return Input::drag(label, &v, stepOr(md, 0.5f));
    default:
        return Input::number(label, &v);
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
            open = BasicWidgets::beginTree(
                categories[g] + "##" + info.typeName + std::to_string(depth) + "_" + std::to_string(g)
            );
        }
        if (open)
        {
            for (auto * prop : groups[g])
            {
                BasicWidgets::pushId(id++);
                if (drawProperty(instance, *prop, depth))
                {
                    changed = true;
                }
                BasicWidgets::popId();
            }
        }
        if (useTree && open)
        {
            BasicWidgets::endTree();
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
        BasicWidgets::pushId(id++);
        const auto & label = action.displayName.empty() ? action.name : action.displayName;
        if (BasicWidgets::button(label + "##" + action.name))
        {
            action.invoke(instance);
        }
        BasicWidgets::tooltip(action.tooltip);
        BasicWidgets::popId();
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
        BasicWidgets::beginDisabled();
    }

    switch (prop.type)
    {
    case PropertyType::Bool: {
        auto v = std::any_cast<bool>(prop.getter(instance));
        changed = BasicWidgets::checkBox(label, &v);
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
            changed = Input::number(label, &v);
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
        changed = Input::text(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec2: {
        auto v = std::any_cast<glm::vec2>(prop.getter(instance));
        changed = Input::vec2(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec3: {
        auto v = std::any_cast<glm::vec3>(prop.getter(instance));
        changed = Input::vec3(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Vec4: {
        auto v = std::any_cast<glm::vec4>(prop.getter(instance));
        changed = Input::vec4(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Mat4: {
        auto v = std::any_cast<glm::mat4>(prop.getter(instance));
        changed = Input::mat4(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Color: {
        auto v = std::any_cast<base::Color>(prop.getter(instance));
        changed = Input::colorPicker(label, v);
        if (changed) prop.setter(instance, v);
        break;
    }
    case PropertyType::Enum:
        // v1 limitation: the concrete enum type is erased inside std::any,
        // so the current value can neither be read nor written generically
        // (std::any_cast requires the exact enum type). Show a fallback.
        BasicWidgets::text(nameFor(prop) + ": <enum not supported>");
        break;
    case PropertyType::Path: {
        auto v = std::any_cast<std::filesystem::path>(prop.getter(instance));
        BasicWidgets::text(nameFor(prop) + ": " + v.string());
        break;
    }
    case PropertyType::Resource:
    default:
        BasicWidgets::text(nameFor(prop) + ": <not supported>");
        break;
    }

    if (readOnly)
    {
        BasicWidgets::endDisabled();
    }

    BasicWidgets::tooltip(prop.metadata.tooltip);
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
        BasicWidgets::beginDisabled();
        BasicWidgets::text(label + ": <not registered>");
        BasicWidgets::endDisabled();
        BasicWidgets::tooltip(prop.metadata.tooltip);
        return false;
    }

    if (depth >= reflection::maxObjectDepth)
    {
        BasicWidgets::text(label + ": <max depth reached>");
        BasicWidgets::tooltip(prop.metadata.tooltip);
        return false;
    }

    bool changed = false;
    if (BasicWidgets::beginTree(label + "##" + prop.name))
    {
        const bool readOnly = prop.isReadOnly();
        if (readOnly)
        {
            BasicWidgets::beginDisabled();
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
            BasicWidgets::endDisabled();
        }
        BasicWidgets::endTree();
    }
    BasicWidgets::tooltip(prop.metadata.tooltip);
    return changed;
}

}
}
