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
#include "command/SceneSettingsCommand.hpp"
#include "command/component/ComponentCommands.hpp"
#include "command/lighting/EnvironmentCommands.hpp"
#include <bg2e/draw/RendererDeferred.hpp>
#include <map>

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
    queueSceneCommand([weak = weak_from_this(), node, enabled]
    {
        auto owner = weak.lock();
        if (!owner)
        {
            throw std::logic_error("Node enabled receiver expired");
        }
        return std::make_unique<command::scene::SetNodeEnabled>(owner->_stage, node, enabled);
    }, effects);
}


void AppDelegate::editSceneSettings(std::shared_ptr<bg2e::draw::RenderSettingsPreferences> preferences,
    std::vector<bg2e::draw::SettingChange> changes, bg2e::draw::RenderSettingsPreferences::Completion completion)
{
    auto delivered = std::make_shared<bool>(false);
    completion = [callback = std::move(completion), delivered](auto result)
    {
        if (!*delivered)
        {
            *delivered = true;
            if (callback)
            {
                callback(std::move(result));
            }
        }
    };
    const auto fail = [completion](const std::string& message)
    {
        if (completion)
        {
            completion({false, {{"scene_edit_rejected", "", message}}});
        }
    };
    if (!preferences || !document() || !document()->commands || commandFaulted() || (_pendingIo && !_settlingUiInteractions) || _imports.busy())
    {
        fail("The document cannot accept scene settings now");
        return;
    }
    const auto expected = document()->commands->history().documentState();
    const auto catalogGeneration = preferences->catalog().generation();
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), preferences, changes = std::move(changes),
        completion, fail, expected, catalogGeneration](auto&)
    {
        auto owner = weak.lock();
        if (!owner || !owner->document() || !owner->document()->commands ||
            owner->document()->generation != expected.generation || preferences->catalog().generation() != catalogGeneration)
        {
            fail("Scene settings document, source or catalog changed");
            return;
        }
        try
        {
            auto validation = preferences->catalog().validate(changes);
            if (!validation.valid())
            {
                if (completion)
                {
                    completion(std::move(validation));
                }
                return;
            }
            std::vector<std::unique_ptr<bg2e::app::Command>> children;
            std::map<bg2e::scene::EnvironmentComponent*, std::pair<std::shared_ptr<bg2e::scene::EnvironmentComponent>,
                std::shared_ptr<bg2e::scene::EnvironmentComponent>>> environments;
            for (const auto& change : changes)
            {
                const auto slash = change.handle.component.find('/');
                const auto id = change.handle.component.substr(slash + 1);
                std::shared_ptr<bg2e::scene::Node> target;
                const auto find = [&](const auto& self, const auto& node) -> void
                {
                    if (node->identifier() == id)
                    {
                        target = node;
                    }
                    for (const auto& child : node->children())
                    {
                        self(self, child);
                    }
                };
                find(find, owner->document()->root);
                if (!target && owner->_stage->fillLightNode()->identifier() == id)
                {
                    target = owner->_stage->fillLightNode();
                }
                const auto descriptor = preferences->catalog().describe(change.handle);
                if (!target || !descriptor.available || !descriptor.property.setter)
                {
                    throw std::invalid_argument("Scene setting receiver is no longer available");
                }
                if (change.handle.component.starts_with("environment/"))
                {
                    auto* component = target->environment();
                    if (!component || !command::supportedComponent(*component))
                    {
                        throw std::invalid_argument("Environment setting requires a built-in component");
                    }
                    auto& pair = environments[component];
                    if (!pair.first)
                    {
                        pair.first = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(component->shared_from_this());
                        pair.second = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(component->clone());
                    }
                    descriptor.property.setter(pair.second.get(), change.value);
                }
                else if (change.handle.component.starts_with("light/") && target->light())
                {
                    auto component = target->light()->shared_from_this();
                    bg2e::ui::ReflectionEditIntent intent;
                    intent.type = descriptor.property.type;
                    intent.proposed = change.value;
                    intent.path = {{component->typeName(), "light", {}, false},
                        {"bg2e::base::Light", change.handle.property, {}, false}};
                    children.push_back(std::make_unique<command::component::EditProperty>(owner->_stage, component, intent));
                }
                else
                {
                    throw std::invalid_argument("Unsupported scene setting receiver");
                }
            }
            for (const auto& [component, pair] : environments)
            {
                auto generator = [weak]() -> command::lighting::EditEnvironment::Generator*
                {
                    auto active = weak.lock();
                    auto* deferred = active ? dynamic_cast<bg2e::draw::RendererDeferred*>(active->renderer()) : nullptr;
                    return deferred ? deferred->environmentGenerator() : nullptr;
                };
                children.push_back(std::make_unique<command::lighting::EditEnvironment>(owner->_stage, pair.first, pair.second, generator));
            }
            auto sourceValid = [weak, preferences = std::weak_ptr<bg2e::draw::RenderSettingsPreferences>(preferences), expected, catalogGeneration]
            {
                auto active = weak.lock();
                auto facade = preferences.lock();
                return active && active->document() && active->document()->commands && facade &&
                    active->document()->generation == expected.generation && facade->catalog().generation() == catalogGeneration;
            };
            auto action = std::make_unique<command::SceneSettingsCommand>(std::move(children), sourceValid);
            command::Effects effects;
            effects.lightmapSources = true;
            effects.renderHistories = true;
            auto finished = [completion](auto result)
            {
                if (completion)
                {
                    bg2e::draw::SettingsResult settings;
                    settings.applied = result.accepted();
                    if (!result.accepted())
                    {
                        settings.issues.push_back({"scene_command_failed", "", std::string(result.message)});
                    }
                    completion(std::move(settings));
                }
            };
            owner->document()->commands->submit(std::move(action), command::Boundary::Conservative, effects,
                [weak, generation = expected.generation]
                {
                    auto active = weak.lock();
                    return active && active->document() && active->document()->generation == generation;
                }, finished);
            owner->document()->commands->pump();
            owner->requestSceneFrame();
        }
        catch (const std::exception& error)
        {
            fail(error.what());
        }
    }, [fail]
    {
        fail("Renderer changed before scene settings preparation");
    });
    requestSceneFrame();
}


bool AppDelegate::historyActionAvailable(bool redo) const
{
    if (!document() || !document()->commands || _pendingIo || _imports.busy() || _importPublicationPending || sceneLoading())
    {
        return false;
    }
    const auto& history = document()->commands->history();
    return !history.faulted() && document()->commands->pending() == 0 &&
        ((redo ? history.redoCount() : history.undoCount()) != 0 || history.transactionActive());
}


std::string AppDelegate::historyActionLabel(bool redo) const
{
    const std::string label = redo ? "Redo" : "Undo";
    if (!document() || !document()->commands)
    {
        return label;
    }
    const auto& history = document()->commands->history();
    if (history.faulted())
    {
        return label + " (history fault)";
    }
    if (document()->commands->pending() || _importPublicationPending || _imports.busy() || _pendingIo || sceneLoading())
    {
        return label + " (busy)";
    }
    const auto name = redo ? history.redoName() : history.undoName();
    return name.empty() ? label : label + " " + name;
}


bool AppDelegate::settleEditDrafts()
{
    if (_settlingUiInteractions)
    {
        return !commandFaulted();
    }
    struct Scope {
        bool& value;
        ~Scope()
        {
            value = false;
        }
    } scope{_settlingUiInteractions};
    _settlingUiInteractions = true;
    if (_settleUiInteractions && !_settleUiInteractions(true) && !_settleUiInteractions(false))
    {
        _status = "The active edit could not be settled";
        return false;
    }
    if (document() && document()->commands && document()->commands->history().transactionActive())
    {
        const auto generation = document()->generation;
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation](auto&)
        {
            if (auto owner = weak.lock(); owner && owner->document() && owner->document()->generation == generation)
            {
                owner->finishTransformInteraction(true);
                owner->document()->commands->pump();
                owner->requestSceneFrame();
            }
        });
    }
    return !commandFaulted();
}


void AppDelegate::undoEdit()
{
    if (document() && document()->commands && !_pendingIo && !sceneLoading() && !_imports.busy() &&
        !_importPublicationPending && settleEditDrafts())
    {
        const auto generation = document()->generation;
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation](auto&)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                return;
            }
            owner->finishTransformInteraction(true);
            owner->document()->commands->undo();
            owner->document()->commands->pump();
            owner->requestSceneFrame();
        });
        requestSceneFrame();
    }
}


void AppDelegate::redoEdit()
{
    if (document() && document()->commands && !_pendingIo && !sceneLoading() && !_imports.busy() &&
        !_importPublicationPending && settleEditDrafts())
    {
        const auto generation = document()->generation;
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation](auto&)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                return;
            }
            owner->finishTransformInteraction(true);
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
    struct UiSettlementScope {
        bool& settling;
        bool previous;
        ~UiSettlementScope()
        {
            settling = previous;
        }
    } scope{_settlingUiInteractions, _settlingUiInteractions};
    // Buffered resource commits prepare in the graphics queue. Keep the IO
    // settlement admission window open until that preparation has drained too.
    _settlingUiInteractions = true;
    if (_settleUiInteractions)
    {
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
        if (previous != 0 && remaining >= previous)
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
    if (commands->retiredResourcesPending())
    {
        _engine->device()->waitIdleForResourceUpdate();
    }
    commands->releaseRetired();
    if (result.accepted())
    {
        processReadActions();
    }
    if (!result.accepted())
    {
        _status = std::string(result.message);
        return allowFault && result.status == bg2e::app::CommandStatus::Faulted &&
            !commands->history().transactionActive();
    }
    return true;
}

}
