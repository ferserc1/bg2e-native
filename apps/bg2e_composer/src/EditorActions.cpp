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
#include "command/material/MaterialCommands.hpp"
#include "command/material/MaterialInteraction.hpp"
#include "command/lighting/EnvironmentCommands.hpp"

#include <bg2e/geo/modifiers.hpp>
#include <bg2e/utils/MaterialModifier.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/scene/TransformComponent.hpp>

#include <fstream>
#include <bg2e/db/scene.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <set>
#include <cmath>

namespace bg2e_composer {

void AppDelegate::editGeometry(command::geometry::Operation operation)
{
    if (!_selection || !document() || !document()->root || commandFaulted() || !settleEditDrafts())
    {
        return;
    }
    try
    {
        std::vector<bg2e::manipulation::SelectionItem> targets;
        for (const auto& item : _selection->selectedItems())
        {
            if (item && item->lockMesh())
            {
                targets.push_back(*item);
            }
        }
        if (targets.empty())
        {
            return;
        }
        command::Effects effects;
        effects.lightmapSources = true;
        effects.renderHistories = true;
        effects.selection = true;
        queueSceneCommand([weak = weak_from_this(), targets = std::move(targets), operation]
        {
            auto owner = weak.lock();
            if (!owner)
            {
                throw std::logic_error("Geometry receiver expired");
            }
            auto stopProducers = [weak]
            {
                if (auto active = weak.lock())
                {
                    active->invalidateLightmapSources();
                }
            };
            return std::make_unique<command::geometry::TransformGeometry>(owner->_stage, targets, operation, stopProducers);
        }, effects);
    }
    catch (const std::exception& error)
    {
        _status = error.what();
    }
}


void AppDelegate::copyMaterial()
{
    if (!_selection || !document() || !settleEditDrafts())
    {
        return;
    }
    const auto item = _selection->selectedItem();
    if (item)
    {
        _deferredReads.push_back({DeferredRead::Kind::CopyMaterial, document()->generation, *item, {}});
        requestSceneFrame();
    }
}


void AppDelegate::processReadActions()
{
    if (!document() || !document()->root || !document()->commands || (_pendingIo && !_settlingUiInteractions) || sceneLoading() || _imports.busy() || _importPublicationPending ||
        commandFaulted() || document()->commands->pending() || document()->commands->history().transactionActive())
    {
        return;
    }
    auto actions = std::move(_deferredReads);
    _deferredReads.clear();
    for (const auto& action : actions)
    {
        if (action.generation != document()->generation)
        {
            continue;
        }
        try
        {
            _engine->device()->waitIdleForResourceUpdate();
            if (action.kind == DeferredRead::Kind::CopyMaterial)
            {
                auto node = action.target.node.lock();
                auto mesh = action.target.lockMesh();
                if (node && _stage->contains(node.get()) && mesh)
                {
                    auto prepared = mesh->drawMaterial(action.target.submesh)->clone();
                    _clipboard = std::move(prepared);
                    _status = "Material copied";
                }
            }
            else
            {
                auto preset = _stage->environmentPreset();
                _status = bg2e::db::saveScene(preset.get(), *_engine, action.path)
                    ? "Environment preset saved" : "Environment preset save failed (see log)";
            }
        }
        catch (const std::exception& error)
        {
            _status = error.what();
        }
    }
}


bool AppDelegate::materialEditIntent(bg2e::ui::EditPhase phase, uint64_t interaction,
    const bg2e::ui::MaterialEditTargets& targets, const bg2e::ui::MaterialEditIntent& intent)
{
    using bg2e::ui::EditPhase;
    if (phase == EditPhase::Cancel && _valueEdit && _valueEdit->interaction == interaction)
    {
        return valueEditIntent(phase, interaction, {}, {});
    }
    if (!document() || !document()->commands)
    {
        return phase == EditPhase::Cancel;
    }
    const bool values = intent.cpuValuesOnly && intent.patch && !intent.sampling && !intent.indirectMode &&
        !intent.ordinaryAo && intent.linearSlot < 0 && intent.clearSlot < 0;
    if (values)
    {
        if (phase != EditPhase::Cancel && targets.rendererGeneration != rendererGeneration())
        {
            return false;
        }
        return valueEditIntent(phase, interaction,
            [stage = std::weak_ptr<StageScene>(_stage), targets, effects = valueEditEffects()]
            {
                return command::material::materialInteraction(stage, targets, effects);
            }, intent);
    }
    if (phase == EditPhase::Cancel)
    {
        return true; // Resource drafts remain private until publication.
    }
    if (!document() || !document()->commands || commandFaulted() ||
        document()->commands->history().transactionActive() || sceneLoading() || _imports.busy() ||
        (_pendingIo && !_settlingUiInteractions) || targets.rendererGeneration != rendererGeneration())
    {
        return false;
    }
    if (phase == EditPhase::Commit)
    {
        try
        {
            queueMaterialEdit(targets, intent);
        }
        catch (...)
        {
            return false;
        }
    }
    return true;
}

void AppDelegate::queueMaterialEdit(bg2e::ui::MaterialEditTargets targets, bg2e::ui::MaterialEditIntent intent,
    std::shared_ptr<bg2e::draw::MaterialBase> paste)
{
    if (!document() || !document()->root || targets.selection.empty() || commandFaulted())
    {
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(),
        [weak = weak_from_this(), generation, targets = std::move(targets), intent = std::move(intent), paste = std::move(paste)](auto&)
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation || owner->commandFaulted())
            {
                return;
            }
            try
            {
                auto action = std::make_unique<command::material::EditMaterials>(owner->_stage, targets, intent, paste);
                command::Effects effects;
                effects.lightmapSources = true;
                effects.renderHistories = true;
                owner->submitNodeCommand(targets.selection.front().node, std::move(action),
                    command::Boundary::Conservative, effects);
            }
            catch (const std::exception& error)
            {
                owner->_status = error.what();
            }
        });
    requestSceneFrame();
}


void AppDelegate::pasteMaterial()
{
    if (!_clipboard || !_selection || !settleEditDrafts())
    {
        return;
    }
    bg2e::ui::MaterialEditTargets targets;
    for (const auto& item : _selection->selectedItems())
    {
        targets.selection.push_back(*item);
    }
    queueMaterialEdit(std::move(targets), {}, _clipboard);
}

void AppDelegate::applyMaterialPatch(const std::filesystem::path& path)
{
    if (!_selection || path.empty() || !settleEditDrafts())
    {
        return;
    }
    try
    {
        std::ifstream input(path);
        if (!input)
        {
            throw std::runtime_error("Could not open material patch");
        }
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const bg2e::utils::MaterialModifier patch(text, path.parent_path());
        if (!patch.isValid())
        {
            throw std::invalid_argument("Invalid material patch");
        }
        bg2e::ui::MaterialEditTargets targets;
        for (const auto& item : _selection->selectedItems())
        {
            targets.selection.push_back(*item);
        }
        bg2e::ui::MaterialEditIntent intent;
        intent.patch = patch;
        queueMaterialEdit(std::move(targets), std::move(intent));
    }
    catch (const std::exception& error)
    {
        _status = error.what();
    }
}

void AppDelegate::queueEnvironmentEdit(std::weak_ptr<bg2e::scene::EnvironmentComponent> target,
    bg2e::ui::ReflectionEditIntent intent)
{
    if (!document() || !document()->commands || commandFaulted())
    {
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation, target, intent = std::move(intent)](auto&)
    {
        auto owner = weak.lock();
        auto live = target.lock();
        if (!owner || !live || !owner->document() || owner->document()->generation != generation ||
            !owner->_stage->contains(live->ownerNode()) || owner->commandFaulted())
        {
            return;
        }
        try
        {
            auto candidate = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(live->clone());
            const auto* type = bg2e::reflection::TypeRegistry::get().type(candidate->typeName());
            void* propertyOwner = nullptr;
            const auto* property = type ? bg2e::ui::ReflectionWidget::resolveProperty(candidate.get(), *type, intent.path, propertyOwner) : nullptr;
            if (!property || !property->setter || property->type != intent.type)
            {
                throw std::invalid_argument("Environment property path changed");
            }
            property->setter(propertyOwner, intent.proposed);
            auto generator = [weak]() -> command::lighting::EditEnvironment::Generator*
            {
                auto active = weak.lock();
                auto* deferred = active ? dynamic_cast<bg2e::draw::RendererDeferred*>(active->renderer()) : nullptr;
                return deferred ? deferred->environmentGenerator() : nullptr;
            };
            auto action = std::make_unique<command::lighting::EditEnvironment>(owner->_stage, live, candidate, generator);
            command::Effects effects;
            effects.lightmapSources = true;
            effects.renderHistories = true;
            owner->submitNodeCommand(live->ownerNode()->shared_from_this(), std::move(action), command::Boundary::Conservative, effects);
        }
        catch (const std::exception& error)
        {
            owner->_status = error.what();
        }
    });
    requestSceneFrame();
}


void AppDelegate::environmentPreset(const std::filesystem::path& path, bool save)
{
    if (!document() || !document()->root || path.empty() || !settleEditDrafts())
    {
        return;
    }
    if (save)
    {
        _deferredReads.push_back({DeferredRead::Kind::SaveEnvironmentPreset, document()->generation, {}, path});
        requestSceneFrame();
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), path, generation](auto&)
    {
        auto owner = weak.lock();
        if (!owner || !document() || document()->generation != generation)
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
            auto loaded = bg2e::db::loadScene(path); // Parsing remains outside the command.
            const auto& children = loaded->rootNode()->children();
            if (children.size() != 1)
            {
                throw std::invalid_argument("Environment preset requires exactly one root");
            }
            auto generator = [weak]() -> command::lighting::EditEnvironment::Generator*
            {
                auto active = weak.lock();
                auto* deferred = active ? dynamic_cast<bg2e::draw::RendererDeferred*>(active->renderer()) : nullptr;
                return deferred ? deferred->environmentGenerator() : nullptr;
            };
            auto action = std::make_unique<command::lighting::AdoptEnvironmentPreset>(_stage, children[0], generator);
            command::Effects effects;
            effects.lightmapSources = true;
            effects.renderHistories = true;
            effects.selection = true;
            submitNodeCommand(document()->root, std::move(action), command::Boundary::Conservative, effects);
        }
        catch (const std::exception& error)
        {
            _status = error.what();
        }
    });
}

void AppDelegate::setHighlight(float intensity)
{
    enqueueCpuValueUpdate(rendererGeneration(), [this, weak = weak_from_this(), intensity](auto&)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        _overlay->setLineIntensity(intensity);
    });
}

}
