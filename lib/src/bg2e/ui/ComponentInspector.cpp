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

#include <bg2e/ui/ComponentInspector.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/ui/Layout.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Value.hpp>

#include <algorithm>
#include <memory>
#include <any>
#include <filesystem>
#include <vector>

namespace bg2e {
namespace ui {

namespace {

struct ResourceSnapshot {
    const reflection::PropertyInfo * property = nullptr;
    std::filesystem::path value;
};

struct ComponentOption {
    std::string typeName;
    std::string displayName;
};

std::vector<ComponentOption> availableComponents(const scene::Node& node)
{
    const auto& reflectionRegistry = reflection::TypeRegistry::get();
    const auto& factoryRegistry = scene::ComponentFactoryRegistry::get();
    const auto& nodeComponents = node.orderedComponents();
    std::vector<ComponentOption> result;

    for (const auto& typeName : reflectionRegistry.typeNames())
    {
        if (!factoryRegistry.contains(typeName))
        {
            continue;
        }

        const bool alreadyAdded = std::any_of(
            nodeComponents.begin(),
            nodeComponents.end(),
            [&typeName](const std::shared_ptr<scene::Component>& component) {
                return component->typeName() == typeName;
            }
        );
        if (alreadyAdded)
        {
            continue;
        }

        const auto* typeInfo = reflectionRegistry.type(typeName);
        result.push_back({
            typeName,
            typeInfo && !typeInfo->displayName.empty() ? typeInfo->displayName : typeName
        });
    }

    std::sort(result.begin(), result.end(), [](const ComponentOption& lhs, const ComponentOption& rhs) {
        return lhs.displayName < rhs.displayName;
    });
    return result;
}

std::filesystem::path resourceValue(
    const reflection::PropertyInfo& property,
    const void * instance
) {
    const auto value = property.getter(instance);
    return property.metadata.resourceValueType == reflection::PropertyType::String
        ? std::filesystem::path(std::any_cast<std::string>(value))
        : std::any_cast<std::filesystem::path>(value);
}

void restoreResource(
    const ResourceSnapshot& snapshot,
    void * instance
) {
    if (snapshot.property->metadata.resourceValueType == reflection::PropertyType::String)
    {
        snapshot.property->setter(instance, snapshot.value.string());
    }
    else
    {
        snapshot.property->setter(instance, snapshot.value);
    }
}

} // anonymous namespace

void ComponentInspector::draw()
{
    if (!_node)
    {
        Text::text("No selection");
        return;
    }

    bool changed = false;

    Text::text(_node->name());

    const auto componentOptions = availableComponents(*_node);
    if (!componentOptions.empty())
    {
        std::vector<std::string> componentNames;
        componentNames.reserve(componentOptions.size());
        for (const auto& option : componentOptions)
        {
            componentNames.push_back(option.displayName);
        }

        if (_selectedComponentIndex >= componentOptions.size())
        {
            _selectedComponentIndex = 0;
        }

        if (Button::button("Add Component", false))
        {
            auto* component = scene::ComponentFactoryRegistry::get().createDefault(
                componentOptions[_selectedComponentIndex].typeName
            );
            if (component)
            {
                _node->addComponent(component);
                changed = true;
            }
        }
        Value::comboBox(componentNames, _selectedComponentIndex, "componentType", true, true);
    }
    else
    {
        Text::text("No components available");
    }
    Text::separator("Components");

    std::shared_ptr<scene::Component> pendingRemove;
    int id = 0;

    for (auto & comp : _node->orderedComponents())
    {
        const auto * typeInfo = reflection::TypeRegistry::get().type(comp->typeName());
        if (!typeInfo)
        {
            continue;
        }

        Group::pushId(id++);

        const auto headerTitle = !typeInfo->displayName.empty()
            ? typeInfo->displayName
            : comp->typeName();

        if (Group::collapsingHeader(headerTitle))
        {
            // Right-aligned remove button on the header row
            auto removeWidth = static_cast<int32_t>(
                Layout::calcButtonWidth("Remove") + Layout::getItemHorizontalSpacing()
            );
            Layout::sameLine(-removeWidth);
            if (Button::button("Remove"))
            {
                // Defer the removal until the iteration is finished
                pendingRemove = comp;
            }

            std::vector<ResourceSnapshot> resources;
            for (const auto & property : typeInfo->properties)
            {
                if (property.type == reflection::PropertyType::Resource && property.getter)
                {
                    resources.push_back({ &property, resourceValue(property, comp.get()) });
                }
            }

            bool componentChanged = ReflectionWidget::drawProperties(comp.get(), *typeInfo);
            size_t changedResources = 0;
            size_t rejectedResources = 0;
            for (const auto & resource : resources)
            {
                const auto selected = resourceValue(*resource.property, comp.get());
                if (selected == resource.value) continue;
                ++changedResources;

                if (_onResourceChanged && !_onResourceChanged(
                        comp.get(), resource.property->name, resource.value, selected))
                {
                    restoreResource(resource, comp.get());
                    ++rejectedResources;
                }
            }

            // A component containing only rejected Resource changes did
            // not change. Preserve notification for any simultaneous
            // non-resource edit.
            if (componentChanged &&
                !(changedResources > 0 &&
                  rejectedResources == changedResources &&
                  resources.size() == typeInfo->properties.size()))
            {
                changed = true;
            }
            ReflectionWidget::drawActions(comp.get(), *typeInfo);
        }

        Group::popId();
    }

    if (pendingRemove)
    {
        _node->removeComponent(pendingRemove);
        changed = true;
    }

    if (changed)
    {
        notifyChanged();
    }
}

void ComponentInspector::notifyChanged() const
{
    if (_onChanged)
    {
        _onChanged();
    }
}

}
}
