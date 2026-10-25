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

#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

#include <algorithm>
#include <any>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace tutorial {

class TutorialNodeEditor {
  public:
    using NodeOperation = std::function<void(bg2e::scene::Node&)>;
    using Queue = std::function<void(NodeOperation)>;

    void draw(bg2e::scene::Node* node, const Queue& queue)
    {
        using namespace bg2e;
        if (!node)
        {
            ui::Text::text("No selection");
            return;
        }
        auto name = node->name();
        auto enabled = node->enabled();
        if (ui::Value::text("Node name", name))
        {
            queue([name](scene::Node& target)
            {
                target.setName(name);
            });
        }
        if (ui::Button::checkBox("Enabled", &enabled))
        {
            queue([enabled](scene::Node& target)
            {
                target.setEnabled(enabled);
            });
        }
        drawAddComponent(*node, queue);
        if (ui::Group::beginTree("Reflection registry"))
        {
            auto& registry = reflection::TypeRegistry::get();
            ui::Text::text("Registered types: " + std::to_string(registry.typeNames().size()));
            for (const auto& component : node->orderedComponents())
            {
                const auto key = component->typeName();
                ui::Text::text(key + (registry.contains(key) ? " (registered)" : " (not registered)") +
                    "; object depth: " + std::to_string(registry.objectChainDepth(key)));
            }
            ui::Group::endTree();
        }

        int id = 0;
        for (const auto& component : node->orderedComponents())
        {
            ui::Group::pushId(id++);
            const auto* info = reflection::TypeRegistry::get().type(component->typeName());
            if (!info)
            {
                ui::Text::text(component->typeName() + ": no reflection metadata");
                ui::Group::popId();
                continue;
            }
            const auto title = info->displayName.empty() ? info->typeName : info->displayName;
            if (ui::Group::collapsingHeader(title))
            {
                if (ui::Button::button("Remove"))
                {
                    queue([component](scene::Node& target)
                    {
                        if (component->ownerNode() == &target)
                        {
                            target.removeComponent(component);
                        }
                    });
                }
                // Retain the component, but recheck membership when the edit executes.
                const ui::ReflectionWidget::EditHandler edit = [queue, component](auto operation, bool)
                {
                    queue([component, operation = std::move(operation)](scene::Node& target)
                    {
                        if (component->ownerNode() == &target)
                        {
                            operation(component.get());
                        }
                    });
                };
                drawProperties(component.get(), *info, edit);
                drawActions(*info, edit);
            }
            ui::Group::popId();
        }
    }

  private:
    static bool editScalar(std::any& value, const bg2e::reflection::PropertyInfo& property)
    {
        using namespace bg2e;
        const auto label = (property.metadata.displayName.empty()
            ? property.name : property.metadata.displayName) + "##" + property.name;
        // The two types used by RotationComponent are implemented explicitly.
        // Other scalar types use the engine's existing value-only widget adapter.
        if (property.type != reflection::PropertyType::Float &&
            property.type != reflection::PropertyType::Enum)
        {
            return ui::ReflectionWidget::editValue(value, property);
        }
        const bool writable = !property.isReadOnly();
        ui::Group::beginDisabled(!writable);
        bool changed = false;
        if (property.type == reflection::PropertyType::Float)
        {
            auto next = std::any_cast<float>(value);
            const auto& metadata = property.metadata;
            if (property.editor == reflection::PropertyEditor::Slider && metadata.min && metadata.max)
            {
                changed = ui::Numeric::sliderFloat(label, &next, float(*metadata.min), float(*metadata.max));
            }
            else if (property.editor == reflection::PropertyEditor::Input)
            {
                changed = ui::Numeric::number(label, &next);
            }
            else
            {
                changed = ui::Numeric::drag(label, &next, float(metadata.step.value_or(0.1)));
            }
            if (changed)
            {
                if (metadata.min)
                {
                    next = std::max(next, float(*metadata.min));
                }
                if (metadata.max)
                {
                    next = std::min(next, float(*metadata.max));
                }
                value = next;
            }
        }
        else
        {
            const auto current = std::any_cast<int64_t>(value);
            std::vector<std::string> labels;
            std::vector<int64_t> values;
            uint32_t selected = 0;
            bool found = false;
            for (const auto& [labelText, enumValue] : property.metadata.enumOptions)
            {
                if (current == enumValue)
                {
                    selected = uint32_t(labels.size());
                    found = true;
                }
                labels.push_back(labelText);
                values.push_back(enumValue);
            }
            if (!found)
            {
                selected = uint32_t(labels.size());
                labels.push_back("Unknown (" + std::to_string(current) + ")");
                values.push_back(current);
            }
            if (property.metadata.enumOptions.empty())
            {
                ui::Text::text(label + ": no enum options");
            }
            else if (ui::Value::comboBox(label, labels, selected))
            {
                value = values[selected];
                changed = true;
            }
        }
        ui::Text::tooltip(property.metadata.tooltip);
        ui::Group::endDisabled();
        return writable && changed;
    }

    static void drawProperties(void* instance, const bg2e::reflection::TypeInfo& info,
        const bg2e::ui::ReflectionWidget::EditHandler& edit)
    {
        using namespace bg2e;
        std::vector<std::string> categories;
        for (const auto& property : info.properties)
        {
            if (std::find(categories.begin(), categories.end(), property.metadata.category) == categories.end())
            {
                categories.push_back(property.metadata.category);
            }
        }
        int id = 0;
        for (const auto& category : categories)
        {
            if (!category.empty())
            {
                ui::Text::separator(category);
            }
            for (const auto& property : info.properties)
            {
                if (property.metadata.category != category)
                {
                    continue;
                }
                ui::Group::pushId(id++);
                if (property.type == reflection::PropertyType::Object ||
                    property.type == reflection::PropertyType::PolymorphicObject)
                {
                    // This adapter handles recursive paths and subtype replacement.
                    // It resolves child pointers again when the queued edit executes.
                    reflection::TypeInfo nested;
                    nested.typeName = info.typeName;
                    nested.properties.push_back(property);
                    nested.properties.front().metadata.category.clear();
                    ui::ReflectionWidget::drawProperties(instance, nested, 0, edit);
                }
                else if (property.getter)
                {
                    auto value = property.getter(instance);
                    if (editScalar(value, property))
                    {
                        edit([setter = property.setter, value = std::move(value)](void* target)
                        {
                            setter(target, value);
                        }, property.type == reflection::PropertyType::Resource ||
                           property.type == reflection::PropertyType::Path);
                    }
                }
                ui::Group::popId();
            }
        }
    }

    static void drawActions(const bg2e::reflection::TypeInfo& info,
        const bg2e::ui::ReflectionWidget::EditHandler& edit)
    {
        using namespace bg2e;
        for (const auto& action : info.actions)
        {
            if (!action.category.empty())
            {
                ui::Text::text(action.category);
            }
            const auto label = (action.displayName.empty() ? action.name : action.displayName)
                + "##action_" + action.name;
            if (ui::Button::button(label))
            {
                edit(action.invoke, true);
            }
            ui::Text::tooltip(action.tooltip);
        }
    }

    void drawAddComponent(bg2e::scene::Node& node, const Queue& queue)
    {
        using namespace bg2e;
        auto& registry = reflection::TypeRegistry::get();
        auto& factories = scene::ComponentFactoryRegistry::get();
        std::vector<std::string> keys;
        std::vector<std::string> labels;
        auto typeNames = registry.typeNames();
        std::sort(typeNames.begin(), typeNames.end());
        for (const auto& key : typeNames)
        {
            const auto& components = node.orderedComponents();
            const bool exists = std::any_of(components.begin(), components.end(), [&](const auto& component)
            {
                return component->typeName() == key;
            });
            if (!factories.contains(key) || exists)
            {
                continue;
            }
            const auto* info = registry.type(key);
            keys.push_back(key);
            labels.push_back(info->displayName.empty() ? key : info->displayName);
        }
        if (keys.empty())
        {
            ui::Text::text("No components available");
            return;
        }
        if (_selectedComponent >= keys.size())
        {
            _selectedComponent = 0;
        }
        ui::Value::comboBox("Component type", labels, _selectedComponent);
        if (ui::Button::button("Add Component"))
        {
            queue([key = keys[_selectedComponent]](scene::Node& target)
            {
                // Another window may have added the component since the click.
                for (const auto& component : target.orderedComponents())
                {
                    if (component->typeName() == key)
                    {
                        return;
                    }
                }
                if (auto* component = scene::ComponentFactoryRegistry::get().createDefault(key))
                {
                    target.addComponent(component);
                }
            });
        }
    }

    uint32_t _selectedComponent = 0;
};

}
