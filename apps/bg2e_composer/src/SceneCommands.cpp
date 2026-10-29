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

#include "AppDelegate.hpp"
#include "command/scene/StructureCommands.hpp"
#include "command/scene/SelectionCommands.hpp"
#include "command/component/ComponentCommands.hpp"

#include <bg2e/app/PreferencesStore.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <stdexcept>
#include <bg2e/manipulation/SelectableComponent.hpp>

namespace bg2e_composer {

bg2e::app::CommandResult AppDelegate::submitSceneCommand(std::unique_ptr<bg2e::app::Command> action,
    command::Effects effects, command::Boundary boundary)
{
    if (!document() || !document()->commands || (_pendingIo && !_settlingUiInteractions) || sceneLoading() || _imports.busy())
    {
        return {bg2e::app::CommandStatus::Rejected, "The document cannot accept this edit now"};
    }
    const auto generation = document()->generation;
    const auto stage = std::weak_ptr<StageScene>(_stage);
    auto result = document()->commands->submit(std::move(action), boundary, effects, [stage, generation]
    {
        auto owner = stage.lock();
        return owner && owner->document().generation == generation;
    });
    if (!result.accepted())
    {
        _status = std::string(result.message);
    }
    document()->commands->pump();
    requestSceneFrame();
    return result;
}


void AppDelegate::queueSceneCommand(std::function<std::unique_ptr<bg2e::app::Command>()> prepare)
{
    if (!document() || commandFaulted() || _pendingIo || sceneLoading() || _imports.busy())
    {
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation, prepare = std::move(prepare)](auto&)
    {
        auto owner = weak.lock();
        if (!owner || !owner->document() || owner->document()->generation != generation || owner->commandFaulted())
        {
            return;
        }
        owner->finishTransformInteraction(true);
        if (owner->commandFaulted())
        {
            return;
        }
        try
        {
            auto action = prepare(); // Detached preparation only; publication belongs to execute.
            if (action)
            {
                command::Effects effects;
                effects.lightmapSources = true;
                effects.renderHistories = true;
                struct SubmissionScope {
                    bool& settling;
                    bool previous;
                    ~SubmissionScope()
                    {
                        settling = previous;
                    }
                } scope{owner->_settlingUiInteractions, owner->_settlingUiInteractions};
                // This preparation request was admitted before pending IO.
                owner->_settlingUiInteractions = true;
                owner->submitSceneCommand(std::move(action), effects);
            }
        }
        catch (const std::exception& error)
        {
            owner->_status = error.what();
        }
        catch (...)
        {
            owner->_status = "Scene command preparation failed";
        }
    });
    requestSceneFrame();
}


std::vector<std::weak_ptr<bg2e::scene::Node>> AppDelegate::selectedSceneBranches() const
{
    return _selection ? _selection->selectedNodes() : std::vector<std::weak_ptr<bg2e::scene::Node>>{};
}


bool AppDelegate::selectionHistoryEnabled() const
{
    return document() && document()->commands && document()->commands->selectionHistoryEnabled();
}


void AppDelegate::toggleSelectionHistory()
{
    auto& preferences = bg2e::app::PreferencesStore::instance().preferences("draw.bg2e_composer.history");
    const bool enabled = !preferences.get<bool>("recordSelection", false);
    preferences.set<bool>("recordSelection", enabled);
    if (document() && document()->commands)
    {
        document()->commands->setSelectionHistoryEnabled(enabled);
    }
}


void AppDelegate::selectPickedItem(bg2e::manipulation::SelectionItem item, bool additive)
{
    std::optional<bg2e::manipulation::SelectionItem> submesh;
    if (item.kind == bg2e::manipulation::PickKind::Drawable && !item.node.expired())
    {
        submesh = item;
    }
    submitSceneCommand(std::make_unique<command::scene::SetSelection>(_stage, _selection,
        item.node, additive, std::move(submesh), item.node.expired()));
}


void AppDelegate::selectTreeNode(std::weak_ptr<bg2e::scene::Node> node, bool additive)
{
    submitSceneCommand(std::make_unique<command::scene::SetSelection>(_stage, _selection, std::move(node), additive));
}


void AppDelegate::selectNode(std::weak_ptr<bg2e::scene::Node> node, uint32_t submesh, bool additive)
{
    auto target = node.lock();
    auto* component = target ? target->drawable() : nullptr;
    auto mesh = component ? component->drawable() : nullptr;
    if (!mesh || submesh >= mesh->submeshesCount())
    {
        return;
    }
    auto* selectable = target->getComponent<bg2e::manipulation::SelectableComponent>();
    if (!selectable)
    {
        return;
    }
    bg2e::manipulation::SelectionItem item;
    item.node = target;
    item.drawable = std::static_pointer_cast<bg2e::scene::DrawableComponent>(component->shared_from_this());
    item.mesh = mesh;
    item.submesh = submesh;
    item.selectionIdentifier = selectable->identifier(submesh);
    selectPickedItem(item, additive);
}


bg2e::app::CommandResult AppDelegate::editSubmeshMetadata(command::scene::MetadataRequest request)
{
    command::Effects effects;
    effects.renderHistories = true;
    effects.lightmapSources = request.field == command::scene::MetadataField::Visibility;
    return submitSceneCommand(std::make_unique<command::scene::EditMetadata>(_stage, std::move(request)),
        effects, command::Boundary::CpuValue);
}


bool AppDelegate::componentEditIntent(const std::shared_ptr<bg2e::scene::Component>& component,
    bg2e::ui::EditPhase phase, const bg2e::ui::ReflectionEditIntent& intent)
{
    using namespace bg2e::ui;
    if (phase == EditPhase::Cancel)
    {
        return true; // Drafts have not mutated the scene.
    }
    if (!document() || !component || (!_stage->contains(component->ownerNode()) && component->ownerNode() != _stage->fillLightNode().get()) ||
        !command::component::supportedProperty(*component, intent))
    {
        _status = "This property requires a dedicated reversible editor adapter";
        return false;
    }
    if (phase == EditPhase::Commit)
    {
        try
        {
            command::Effects effects;
            effects.lightmapSources = true;
            effects.renderHistories = true;
            return submitSceneCommand(std::make_unique<command::component::EditProperty>(_stage, component, intent), effects).accepted();
        }
        catch (...)
        {
            _status = "Component property command admission failed";
            return false;
        }
    }
    return !commandFaulted();
}


void AppDelegate::componentAction(const std::shared_ptr<bg2e::scene::Component>& component,
    const bg2e::ui::ReflectionActionIntent& intent)
{
    using command::component::Action;
    Action action;
    if (!component || !intent.objectPath.empty() || !command::supportedComponent(*component))
    {
        _status = "This action requires a dedicated reversible editor adapter";
        return;
    }
    if (dynamic_cast<bg2e::scene::TransformComponent*>(component.get()) && intent.action == "setIdentity")
    {
        action = Action::TransformIdentity;
    }
    else if (dynamic_cast<bg2e::scene::CameraComponent*>(component.get()) && intent.action == "resetProjection")
    {
        action = Action::ResetProjection;
    }
    else if (dynamic_cast<bg2e::scene::OrbitCameraComponent*>(component.get()) && intent.action == "reset")
    {
        action = Action::ResetOrbit;
    }
    else
    {
        _status = "This action requires a dedicated reversible editor adapter";
        return;
    }
    command::Effects effects;
    effects.lightmapSources = true;
    effects.renderHistories = true;
    submitSceneCommand(std::make_unique<command::component::EditProperty>(_stage, component,
        bg2e::ui::ReflectionEditIntent{}, action), effects);
}

}
