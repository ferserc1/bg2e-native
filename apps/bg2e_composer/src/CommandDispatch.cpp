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
#include "command/scene/NodeCommands.hpp"

#include <stdexcept>
#include <bg2e/app/PreferencesStore.hpp>
#include <utility>

namespace bg2e_composer {

std::shared_ptr<command::Dispatcher> AppDelegate::makeCommandDispatcher(uint64_t generation)
{
    const auto weak = weak_from_this();
    auto dispatcher = std::make_shared<command::Dispatcher>(generation, true,
        [weak, generation](command::Boundary boundary, const auto* context, auto run, auto cancel)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation ||
                (context && context->generation != generation))
            {
                cancel();
                return;
            }
            const auto binding = owner->rendererGeneration();
            auto operation = [weak, generation, run = std::move(run), cancel](bg2e::draw::Renderer& renderer)
            {
                auto active = weak.lock();
                if (!active || !active->document() || active->document()->generation != generation)
                {
                    cancel();
                    return;
                }
                struct RendererScope {
                    bg2e::draw::Renderer*& slot;
                    bg2e::draw::Renderer* previous;

                    ~RendererScope()
                    {
                        slot = previous;
                    }
                } scope{active->_commandRenderer, active->_commandRenderer};
                active->_commandRenderer = &renderer;
                run();
                active->_stage->synchronizeDirty();
                active->requestSceneFrame(); // Continue a serialized queue on the next prepare tick.
            };
            if (boundary == command::Boundary::CpuValue)
            {
                owner->enqueueCpuValueUpdate(binding, std::move(operation), std::move(cancel));
            }
            else if (boundary == command::Boundary::RetainedResource)
            {
                owner->enqueueRetainedResourceUpdate(binding, std::move(operation), std::move(cancel));
            }
            else
            {
                owner->enqueueRendererUpdate(binding, std::move(operation), std::move(cancel));
            }
        },
        [weak](const command::Effects& effects)
        {
            auto owner = weak.lock();
            if (!owner || !owner->_commandRenderer)
            {
                throw std::logic_error("Composer effects require an active safe execution boundary");
            }
            owner->applyCommandEffects(*owner->_commandRenderer, effects);
        },
        [weak, generation](bg2e::app::CommandResult result)
        {
            if (auto owner = weak.lock(); owner && owner->document() && owner->document()->generation == generation)
            {
                owner->_stage->synchronizeDirty();
                owner->sampleNavigation(false);
                if (!result.accepted())
                {
                    owner->_status = std::string(result.message);
                }
                else if (result.status == bg2e::app::CommandStatus::Applied)
                {
                    owner->_status.clear();
                }
                owner->requestSceneFrame();
            }
        });
    dispatcher->setSelectionHistoryEnabled(bg2e::app::PreferencesStore::instance()
        .preferences("draw.bg2e_composer.history").get<bool>("recordSelection", false));
    return dispatcher;
}


bg2e::app::CommandResult AppDelegate::submitNodeCommand(std::weak_ptr<bg2e::scene::Node> node,
    std::unique_ptr<bg2e::app::Command> action, command::Boundary boundary, command::Effects effects)
{
    if (!document() || !document()->commands || (_pendingIo && !_settlingUiInteractions) || sceneLoading() || _imports.busy())
    {
        return {bg2e::app::CommandStatus::Rejected, "The document cannot accept this edit now"};
    }
    const auto generation = document()->generation;
    const auto weakStage = std::weak_ptr<StageScene>(_stage);
    const auto result = document()->commands->submit(std::move(action), boundary, effects,
        [weakStage, node, generation]
        {
            auto stage = weakStage.lock();
            auto target = node.lock();
            return stage && target && stage->document().generation == generation &&
                (stage->contains(target.get()) || target == stage->fillLightNode());
        });
    if (!result.accepted())
    {
        _status = std::string(result.message);
    }
    document()->commands->pump();
    requestSceneFrame();
    return result;
}


bg2e::app::CommandResult AppDelegate::renameNode(std::weak_ptr<bg2e::scene::Node> node, const std::string& name)
{
    return submitNodeCommand(node, std::make_unique<command::scene::RenameNode>(node, name),
        command::Boundary::CpuValue, {});
}


void AppDelegate::setNodeEnabled(std::weak_ptr<bg2e::scene::Node> node, bool enabled)
{
    command::Effects effects;
    effects.lightmapSources = true;
    effects.renderHistories = true;
    effects.selection = true;
    submitNodeCommand(node, std::make_unique<command::scene::SetNodeEnabled>(_stage, node, enabled),
        command::Boundary::Conservative, effects);
}


void AppDelegate::undoEdit()
{
    if (document() && document()->commands && !_pendingIo && !sceneLoading() && !_imports.busy())
    {
        const auto generation = document()->generation;
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation](auto&)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                return;
            }
            if (owner->_settleUiInteractions && !owner->_settleUiInteractions(false))
            {
                owner->_status = "The active UI edit could not be cancelled";
                return;
            }
            owner->finishTransformInteraction(false);
            owner->document()->commands->undo();
            owner->document()->commands->pump();
            owner->requestSceneFrame();
        });
        requestSceneFrame();
    }
}


void AppDelegate::redoEdit()
{
    if (document() && document()->commands && !_pendingIo && !sceneLoading() && !_imports.busy())
    {
        const auto generation = document()->generation;
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation](auto&)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                return;
            }
            if (owner->_settleUiInteractions && !owner->_settleUiInteractions(false))
            {
                owner->_status = "The active UI edit could not be cancelled";
                return;
            }
            owner->finishTransformInteraction(false);
            owner->document()->commands->redo();
            owner->document()->commands->pump();
            owner->requestSceneFrame();
        });
        requestSceneFrame();
    }
}


void AppDelegate::applyCommandEffects(bg2e::draw::Renderer& renderer, const command::Effects& effects)
{
    if (effects.lightmapSources)
    {
        invalidateLightmapSources();
    }
    if (effects.tools)
    {
        _stage->prepareTools();
    }
    if (effects.sceneReferences)
    {
        _stage->refreshCamera(true);
    }
    if (effects.selection && _selection)
    {
        _selection->refreshSelection();
    }
    if (effects.renderHistories)
    {
        renderer.invalidateHistories();
    }
    requestSceneFrame();
}


void AppDelegate::sampleNavigation(bool trackChanges)
{
    auto* orbit = _stage ? _stage->orbitCamera() : nullptr;
    auto previous = _observedOrbit.lock();
    if (!orbit)
    {
        _observedOrbit.reset();
        return;
    }
    if (trackChanges && previous.get() == orbit &&
        (orbit->rotation() != _observedRotation || orbit->center() != _observedCenter ||
            orbit->distance() != _observedDistance))
    {
        // Includes continuous keyboard navigation evaluated between input events.
        // This is dirty tracking only; it must never manufacture history entries.
        _stage->markModified(bg2e::app::CommandCompatibility::Preserve);
        invalidateLightmapSources();
    }
    _observedOrbit = std::dynamic_pointer_cast<bg2e::scene::OrbitCameraComponent>(orbit->shared_from_this());
    _observedRotation = orbit->rotation();
    _observedCenter = orbit->center();
    _observedDistance = orbit->distance();
}


bool AppDelegate::flushDocumentCommands(bool commit, bool allowFault)
{
    if (!document() || !document()->commands)
    {
        return true;
    }
    auto commands = document()->commands;
    if (_settleUiInteractions)
    {
        struct UiSettlementScope {
            bool& settling;
            ~UiSettlementScope()
            {
                settling = false;
            }
        } scope{_settlingUiInteractions};
        _settlingUiInteractions = true;
        if (!_settleUiInteractions(commit && !commands->history().faulted()))
        {
            _status = "The active UI edit could not be settled";
            return false;
        }
    }
    finishTransformInteraction(commit);
    if (commands->history().faulted())
    {
        commands->pump();
        _stage->synchronizeDirty();
        return allowFault && !commands->history().transactionActive();
    }
    // Drain legacy renderer requests too. Each queue pump schedules only its
    // head, so repeat with a strict progress bound rather than spinning.
    size_t previous = commands->pending();
    do
    {
        commands->pump();
        DefaultRenderLoopDelegate::prepareFrame();
        const auto remaining = commands->pending();
        if (remaining == 0)
        {
            break;
        }
        if (remaining >= previous)
        {
            _status = "Document commands could not reach a safe boundary";
            return false;
        }
        previous = remaining;
    } while (true);
    // Input callbacks in the graphics queue may have begun a drag while draining.
    finishTransformInteraction(commit);
    auto* previousRenderer = _commandRenderer;
    _commandRenderer = renderer();
    bg2e::app::CommandResult result;
    try
    {
        result = commands->settle(commit);
    }
    catch (...)
    {
        _commandRenderer = previousRenderer;
        throw;
    }
    _commandRenderer = previousRenderer;
    _stage->synchronizeDirty();
    sampleNavigation(false);
    commands->releaseRetired(); // Initial mementos contain CPU values/weak identities only.
    if (!result.accepted())
    {
        _status = std::string(result.message);
        return allowFault && result.status == bg2e::app::CommandStatus::Faulted &&
            !commands->history().transactionActive();
    }
    return true;
}

}
