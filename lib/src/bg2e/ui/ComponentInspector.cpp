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

#include <bg2e/app/MainLoop.hpp>

#include <algorithm>
#include <stdexcept>
#include <memory>
#include <any>
#include <filesystem>
#include <vector>

namespace bg2e {
namespace ui {

namespace {

struct ResourceSnapshot {
    const reflection::PropertyInfo* property = nullptr;
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
            [&typeName](const std::shared_ptr<scene::Component>& component)
            {
                return component->typeName() == typeName;
            });
        if (alreadyAdded)
        {
            continue;
        }

        const auto* typeInfo = reflectionRegistry.type(typeName);
        result.push_back({typeName,
                          typeInfo && !typeInfo->displayName.empty() ? typeInfo->displayName : typeName});
    }

    std::sort(result.begin(), result.end(), [](const ComponentOption& lhs, const ComponentOption& rhs)
              {
                  return lhs.displayName < rhs.displayName;
              });
    return result;
}

std::filesystem::path resourceValue(
    const reflection::PropertyInfo& property,
    const void* instance)
{
    const auto value = property.getter(instance);
    return property.metadata.resourceValueType == reflection::PropertyType::String
               ? std::filesystem::path(std::any_cast<std::string>(value))
               : std::any_cast<std::filesystem::path>(value);
}

void restoreResource(
    const ResourceSnapshot& snapshot,
    void* instance)
{
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

ComponentInspector::ComponentInspector(const ComponentInspector& other)
{
    *this = other;
}


ComponentInspector& ComponentInspector::operator=(const ComponentInspector& other)
{
    if (this != &other)
    {
        cancelInteraction();
        if (_editSession && _editSession->active())
        {
            throw std::logic_error("Settle the component interaction before assigning the inspector");
        }
        _node = other._node;
        _selectedComponentIndex = other._selectedComponentIndex;
        _registryRevision = other._registryRevision;
        _onChanged = other._onChanged;
        _onResourceChanged = other._onResourceChanged;
        _onAddRequested = other._onAddRequested;
        _onRemoveRequested = other._onRemoveRequested;
        _onDrawComponent = other._onDrawComponent;
        _onActionRequested = other._onActionRequested;
        onEditRequested(other._onEditRequested);
    }
    return *this;
}


void ComponentInspector::setNode(scene::Node* node)
{
    if (_node != node)
    {
        cancelInteraction();
        if (_editSession && _editSession->active())
        {
            throw std::logic_error("Settle the component interaction before rebinding");
        }
        _node = node;
        _selectedComponentIndex = 0;
    }
}


void ComponentInspector::cancelInteraction()
{
    if (_editSession)
    {
        _editSession->cancel();
    }
}


bool ComponentInspector::commitInteraction()
{
    return !_editSession || _editSession->commit();
}


void ComponentInspector::onEditRequested(EditIntentHandler handler)
{
    cancelInteraction();
    if (_editSession && _editSession->active())
    {
        throw std::logic_error("Settle the component interaction before changing mutation owner");
    }
    _editSession.reset();
    _onEditRequested = handler;
    if (handler)
    {
        _editSession = std::make_unique<EditSession>(
            [this, handler = std::move(handler)](EditPhase phase, uint64_t id, const auto&, const std::any& value)
            {
                const auto& intent = std::any_cast<const ReflectionEditIntent&>(value);
                if (phase == EditPhase::Begin)
                {
                    _editTarget.reset();
                    if (_node && !intent.path.empty())
                    {
                        for (const auto& component : _node->orderedComponents())
                        {
                            if (component->typeName() == intent.path.front().ownerType)
                            {
                                _editTarget = component;
                                break;
                            }
                        }
                    }
                }
                auto target = _editTarget.lock();
                if (!target && phase != EditPhase::Cancel)
                {
                    return false;
                }
                return handler(target, phase, id, intent);
            });
    }
}


void ComponentInspector::onActionRequested(ActionIntentHandler handler)
{
    _onActionRequested = std::move(handler);
}


void ComponentInspector::draw()
{
    if (_editSession)
    {
        _editSession->beginFrame();
    }
    struct SessionScope {
        std::unique_ptr<EditSession>& session;
        ~SessionScope()
        {
            if (session)
            {
                session->endFrame();
            }
        }
    } scope{_editSession};
    const auto revision = reflection::TypeRegistry::get().revision();
    if (_registryRevision != revision)
    {
        _registryRevision = revision;
        _selectedComponentIndex = 0;
    }
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
            cancelInteraction();
            if (_editSession && _editSession->active())
            {
                return;
            }
            if (_onAddRequested)
            {
                _onAddRequested(componentOptions[_selectedComponentIndex].typeName);
            }
            else
            {
                auto* component = scene::ComponentFactoryRegistry::get().createDefault(
                    componentOptions[_selectedComponentIndex].typeName);
                if (component)
                {
                    _node->addComponent(component);
                    changed = true;
                }
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

    for (auto& comp : _node->orderedComponents())
    {
        const auto* typeInfo = reflection::TypeRegistry::get().type(comp->typeName());
        if (!typeInfo)
        {
            continue;
        }

        Group::pushId(id++);

        const auto headerTitle = !typeInfo->displayName.empty()
                                     ? typeInfo->displayName
                                     : comp->typeName();

        // allowOverlap: the Remove button is drawn on top of the header row
        // (sameLine), so the header must not steal its mouse clicks.
        if (Group::collapsingHeader(headerTitle, true, true))
        {
            // Right-aligned remove button on the header row
            auto removeWidth = static_cast<int32_t>(
                Layout::calcButtonWidth("Remove") + Layout::getItemHorizontalSpacing());
            Layout::sameLine(-removeWidth);
            if (Button::button("Remove"))
            {
                // Defer the removal until the iteration is finished
                pendingRemove = comp;
            }

            if (_onDrawComponent)
            {
                _onDrawComponent(comp);
                Group::popId();
                continue;
            }

            if (_editSession)
            {
                std::function<void(const ReflectionActionIntent&)> action;
                if (_onActionRequested)
                {
                    action = [this, comp](const auto& intent)
                    {
                        cancelInteraction();
                        if (!_editSession->active())
                        {
                            _onActionRequested(comp, intent);
                        }
                    };
                }
                ReflectionWidget::drawEditIntents(comp.get(), *typeInfo, *_editSession, action);
                Group::popId();
                continue;
            }

            std::vector<ResourceSnapshot> resources;
            for (const auto& property : typeInfo->properties)
            {
                if (property.type == reflection::PropertyType::Resource && property.getter)
                {
                    resources.push_back({&property, resourceValue(property, comp.get())});
                }
            }

            bool componentChanged = ReflectionWidget::drawProperties(comp.get(), *typeInfo);
            size_t changedResources = 0;
            size_t rejectedResources = 0;
            for (const auto& resource : resources)
            {
                const auto selected = resourceValue(*resource.property, comp.get());
                if (selected == resource.value)
                {
                    continue;
                }
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
        cancelInteraction();
        if (_editSession && _editSession->active())
        {
            return;
        }
        if (_onRemoveRequested)
        {
            _onRemoveRequested(pendingRemove);
        }
        else
        {
            // Resource removal runs only after the device is idle.
            auto node = _node->shared_from_this();
            app::MainLoop::current()->safeUpdateScene([node, pendingRemove]()
            {
                node->removeComponent(pendingRemove);
            });
            changed = true;
        }
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
