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
#include <bg2e/ui/ReflectionWidget.hpp>

#include <functional>
#include <memory>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

namespace bg2e {
namespace scene {
class Component;
class Node;
}
namespace ui {

// Inspector widget for the reflected components of a scene node. Components
// with reflection metadata remain visible even when they expose no editable
// properties, because they may provide essential scene behavior.
class BG2E_API ComponentInspector {
  public:
    ComponentInspector() = default;
    // Copies configuration, never an active preview or its target ownership.
    ComponentInspector(const ComponentInspector& other);
    ComponentInspector& operator=(const ComponentInspector& other);

    using ResourceChangedCallback = std::function<bool(
        scene::Component* component,
        const std::string& propertyName,
        const std::filesystem::path& previousPath,
        const std::filesystem::path& selectedPath)>;

    void setNode(scene::Node* node);
    scene::Node* node() const
    {
        return _node;
    }

    void draw();

    // Optional property intent owner; custom onDrawComponent overrides this
    // drawer. Actions need a separate reversible/nonhistorical adapter. This mode
    // never invokes property setters or onChanged. Cancel before hiding/rebinding
    // or destruction; callbacks follow EditSession's no-throw/recovery contract.
    using EditIntentHandler = std::function<bool(const std::shared_ptr<scene::Component>&,
        EditPhase, uint64_t, const ReflectionEditIntent&)>;
    using ActionIntentHandler = std::function<void(const std::shared_ptr<scene::Component>&,
        const ReflectionActionIntent&)>;
    void onEditRequested(EditIntentHandler handler);
    void onActionRequested(ActionIntentHandler handler);
    void cancelInteraction();
    bool commitInteraction();
    // Includes ownership retained during pending or failed cancellation.
    bool interactionActive() const;

    // Fired when any property changes or a component is removed
    void onChanged(std::function<void()> cb)
    {
        _onChanged = cb;
    }
    void onResourceChanged(ResourceChangedCallback cb)
    {
        _onResourceChanged = std::move(cb);
    }

    // Optional intent mode: the application queues mutations at its safe boundary.
    // Empty callbacks preserve the production inspector's immediate behavior.
    void onAddRequested(std::function<void(const std::string&)> cb)
    {
        _onAddRequested = std::move(cb);
    }
    void onRemoveRequested(std::function<void(std::shared_ptr<scene::Component>)> cb)
    {
        _onRemoveRequested = std::move(cb);
    }
    void onDrawComponent(std::function<void(std::shared_ptr<scene::Component>)> cb)
    {
        _onDrawComponent = std::move(cb);
    }

  protected:
    std::unique_ptr<EditSession> _editSession;
    std::weak_ptr<scene::Component> _editTarget;
    ActionIntentHandler _onActionRequested;
    EditIntentHandler _onEditRequested;
    scene::Node* _node = nullptr;
    uint32_t _selectedComponentIndex = 0;
    uint64_t _registryRevision = 0;
    std::function<void()> _onChanged;
    ResourceChangedCallback _onResourceChanged;
    std::function<void(const std::string&)> _onAddRequested;
    std::function<void(std::shared_ptr<scene::Component>)> _onRemoveRequested;
    std::function<void(std::shared_ptr<scene::Component>)> _onDrawComponent;

    void notifyChanged() const;
};

}
}
