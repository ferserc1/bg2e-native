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
#include "command/component/ComponentCommands.hpp"
#include "command/scene/ReplaceDrawable.hpp"
#include <bg2e/db/mesh_bg2.hpp>

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/MessageBox.hpp>
#include <bg2e/scene/DrawableInstanceComponent.hpp>
#include <bg2e/scene/drawable_resources.hpp>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace bg2e_composer {
namespace {

std::shared_ptr<bg2e::scene::Node> selectedNode(const AppDelegate& app)
{
    auto selection = app.selection();
    auto* selected = selection ? selection->selectedNode() : nullptr;
    return selected && app.stage() && app.stage()->contains(selected) ? selected->shared_from_this() : nullptr;
}




bg2e::ui::DrawableSubmeshInspection inspectSubmesh(const bg2e::scene::DrawableRenderComponent& renderable, uint32_t sub)
{
    bg2e::ui::DrawableSubmeshInspection result;
    result.name = renderable.submeshName(sub);
    result.group = renderable.submeshGroupName(sub);
    result.visible = renderable.submeshVisibility(sub);
    if (const auto* instance = dynamic_cast<const bg2e::scene::DrawableInstanceComponent*>(&renderable))
    {
        result.transform = instance->localSubmeshTransform(sub);
    }
    else if (const auto* owner = dynamic_cast<const bg2e::scene::DrawableComponent*>(&renderable); owner && owner->drawable())
    {
        result.transform = owner->drawable()->localSubmeshTransform(sub);
    }
    using Attributes = bg2e::base::MaterialAttributes;
    const auto& material = renderable.effectiveMaterial(sub);
    const auto color = material.albedo();
    std::ostringstream description;
    description << "Material: " << material.name() << " | Albedo: " << color.r << ", " << color.g << ", " << color.b << ", " << color.a
        << " | Metalness: " << material.metalness() << " | Roughness: " << material.roughness()
        << " | Emission: " << material.lightEmission()
        << " | Transparent: " << (material.isTransparent() ? "yes" : "no")
        << " | Double sided: " << (material.doubleSided() ? "yes" : "no")
        << " | Unlit: " << (material.isUnlit() ? "yes" : "no")
        << " | Indirect: " << (material.indirectTextureMode() == Attributes::IndirectTextureMode::BakedGlobalIllumination ? "baked GI" :
            material.indirectTextureMode() == Attributes::IndirectTextureMode::BakedAmbientOcclusion ? "baked AO" : "AO");
    result.material = description.str();
    using Getter = std::shared_ptr<bg2e::base::Texture> (Attributes::*)() const;
    const Getter getters[]{&Attributes::albedoTexture, &Attributes::metalnessTexture, &Attributes::roughnessTexture,
        &Attributes::normalTexture, &Attributes::aoTexture, &Attributes::lightEmissionTexture};
    const char* names[]{"Albedo", "Metalness", "Roughness", "Normal", "Indirect / lightmap", "Emission"};
    const uint32_t uv[]{material.albedoUVSet(), material.metalnessUVSet(), material.roughnessUVSet(),
        material.normalUVSet(), material.aoUVSet(), material.lightEmissionUVSet()};
    for (size_t slot = 0; slot < 6; ++slot)
    {
        const auto texture = (material.*getters[slot])();
        if (texture)
        {
            result.images.push_back(std::string(names[slot]) + " (UV " + std::to_string(uv[slot]) + "): " +
                (texture->type() == bg2e::base::Texture::TypeFilesystem ? texture->imageFilePath() : "Procedural image"));
        }
    }
    return result;
}

}

const bg2e::ui::DrawableBrowserSnapshot& AppDelegate::drawableBrowserSnapshot() const
{
    return _drawableBrowser;
}


void AppDelegate::refreshDrawableBrowser() noexcept
try
{
    bg2e::ui::DrawableBrowserSnapshot snapshot;
    if (document() && document()->root)
    {
        snapshot.generation = document()->generation;
        snapshot.preferred = _lastLoadedDrawable;
        std::vector<std::shared_ptr<bg2e::draw::DrawableCache>> contexts;
        bg2e::scene::collectDrawableCacheContexts(*document()->root, contexts);
        size_t scopeIndex = 0;
        for (const auto& cache : contexts)
        {
            const auto scope = cache == document()->root->drawableCacheContext() ? std::string("Document cache") :
                "Retained cache " + std::to_string(++scopeIndex);
            for (const auto& entry : cache->entries())
            {
                const auto unload = cache->canUnload(entry.handle);
                snapshot.entries.push_back({entry.handle, entry.displayName, entry.sourcePath, entry.exportPath,
                    entry.usage, entry.geometryRevision, unload.ok(), unload.message, scope});
            }
        }
        if (const auto node = selectedNode(*this))
        {
            auto& inspected = snapshot.selection;
            inspected.nodeName = node->name();
            const auto* owner = node->drawable();
            const auto* instance = node->drawableInstance();
            inspected.owner = owner && owner->isSceneGeometry() && owner->requiresBg2Export();
            inspected.instance = instance != nullptr;
            inspected.removable = inspected.instance && node != document()->root;
            if (const auto* renderable = node->drawableRender(); renderable && (inspected.owner || inspected.instance))
            {
                inspected.handle = instance ? instance->resourceHandle() : owner->resourceHandle();
                if (inspected.owner)
                {
                    inspected.transform = owner->transform();
                }
                inspected.tracing = renderable->rayTracingEnabled();
                for (uint32_t sub = 0; sub < renderable->submeshesCount(); ++sub)
                {
                    inspected.submeshes.push_back(inspectSubmesh(*renderable, sub));
                }
            }
        }
    }
    _drawableBrowser = std::move(snapshot);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "Cannot refresh drawable browser: %s\n", error.what());
}
catch (...)
{
    std::fprintf(stderr, "Cannot refresh drawable browser; previous snapshot retained.\n");
}


void AppDelegate::loadDrawableIntoCache()
try
{
    if (!sceneResourceOperationAvailable())
    {
        return;
    }
    const auto path = bg2e::app::FileDialog::getOpenFilePath({{"BG2 model", "bg2,vwglb"}});
    if (path.empty())
    {
        return;
    }
    PendingIo operation;
    operation.kind = PendingIo::Kind::LoadDrawable;
    operation.path = path;
    operation.generation = document()->generation;
    _pendingIo = std::move(operation);
    requestSceneFrame();
}
catch (const std::exception& error)
{
    bg2e::app::MessageBox::showError("Load drawable", error.what());
}
catch (...)
{
    bg2e::app::MessageBox::showError("Load drawable", "The file picker could not prepare a drawable load.");
}


void AppDelegate::unloadCachedDrawable(bg2e::draw::DrawableResourceHandle handle)
{
    if (!sceneResourceOperationAvailable())
    {
        return;
    }
    PendingIo operation;
    operation.kind = PendingIo::Kind::UnloadDrawable;
    operation.drawable = std::move(handle);
    operation.generation = document()->generation;
    _pendingIo = std::move(operation);
    requestSceneFrame();
}


void AppDelegate::queueDrawableCommand(std::function<std::unique_ptr<bg2e::app::Command>()> prepare)
{
    if (!sceneResourceOperationAvailable() || !settleEditDrafts())
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
        try
        {
            owner->finishTransformInteraction(true);
            if (owner->commandFaulted())
            {
                throw std::runtime_error("The active edit could not be settled; previous scene retained");
            }
            auto action = prepare();
            if (!action)
            {
                return;
            }
            auto reported = std::make_shared<bool>(false);
            const auto completion = [weak, reported](bg2e::app::CommandResult result)
            {
                if (!result.accepted() && !*reported)
                {
                    *reported = true;
                    if (auto app = weak.lock())
                    {
                        try
                        {
                            app->_status = std::string(result.message);
                            bg2e::app::MessageBox::showError("Drawable instance operation", app->_status);
                        }
                        catch (...)
                        {
                            std::fprintf(stderr, "Drawable instance command rejected; could not display its diagnostic.\n");
                        }
                    }
                }
            };
            auto stage = std::weak_ptr<StageScene>(owner->_stage);
            const auto result = owner->document()->commands->submit(std::move(action), command::Boundary::Conservative,
                {.lightmapSources = true, .renderHistories = true, .selection = true}, [stage, generation]
            {
                const auto current = stage.lock();
                return current && current->document().generation == generation;
            }, completion);
            completion(result);
            owner->document()->commands->pump();
            owner->requestSceneFrame();
        }
        catch (const std::exception& error)
        {
            owner->_status = error.what();
            bg2e::app::MessageBox::showError("Drawable instance operation", error.what());
        }
        catch (...)
        {
            owner->_status = "The instance operation could not be prepared; previous scene retained";
            bg2e::app::MessageBox::showError("Drawable instance operation", owner->_status);
        }
    });
    requestSceneFrame();
}


bool AppDelegate::canCreateDrawableInstanceFromOwner() const
{
    const auto node = selectedNode(*this);
    const auto* owner = node ? node->drawable() : nullptr;
    return sceneResourceOperationAvailable() && owner && owner->isSceneGeometry() && owner->requiresBg2Export() &&
        owner->resourceHandle().valid();
}


void AppDelegate::createDrawableInstance(bg2e::draw::DrawableResourceHandle handle)
{
    auto parent = selectedNode(*this);
    if (!parent && document())
    {
        parent = document()->root;
    }
    queueDrawableCommand([this, handle, destination = std::weak_ptr<bg2e::scene::Node>(parent)]() -> std::unique_ptr<bg2e::app::Command>
    {
        auto parent = destination.lock();
        auto cache = handle.context();
        if (!parent || !_stage->contains(parent.get()) || !cache || &cache->engine() != _engine)
        {
            throw std::invalid_argument("The insertion parent or drawable resource is no longer available");
        }
        const auto info = cache->inspect(handle);
        if (!info)
        {
            throw std::invalid_argument("The selected drawable is no longer resident");
        }
        auto child = std::make_shared<bg2e::scene::Node>(info->displayName + " instance");
        auto instance = std::make_shared<bg2e::scene::DrawableInstanceComponent>();
        const auto assigned = instance->setResource(*cache, handle);
        if (!assigned.ok())
        {
            throw std::runtime_error(assigned.message);
        }
        child->addComponent(instance);
        child->addComponent(std::make_shared<bg2e::scene::TransformComponent>());
        try
        {
            _stage->prepareCommandCandidate(child);
            _stage->placeNewNode(*child, *parent);
            return std::make_unique<command::scene::EditNodes>(_stage, _selection,
                command::scene::NodeEdit::Insert, std::vector{child}, parent, "Create drawable instance",
                std::shared_ptr<bg2e::scene::Node>{}, true);
        }
        catch (...)
        {
            _stage->discardCandidate(child);
            throw;
        }
    });
}


void AppDelegate::createDrawableInstanceFromOwner()
{
    const auto source = selectedNode(*this);
    queueDrawableCommand([this, source = std::weak_ptr<bg2e::scene::Node>(source)]() -> std::unique_ptr<bg2e::app::Command>
    {
        const auto original = source.lock();
        const auto* owner = original ? original->drawable() : nullptr;
        if (!original || !_stage->contains(original.get()) || !owner || !owner->requiresBg2Export())
        {
            throw std::invalid_argument("Select an authored drawable owner to create an instance");
        }
        const auto handle = owner->resourceHandle();
        const auto cache = handle.context();
        const auto drawable = owner->drawable();
        if (!cache || &cache->engine() != _engine || !drawable || !owner->resourcesLoaded())
        {
            throw std::invalid_argument("The owner's drawable resource and materials must be loaded before creating an instance");
        }
        auto parent = original->parent() ? original->parent()->shared_from_this() : original;
        auto child = std::make_shared<bg2e::scene::Node>(original->name() + " instance");
        auto instance = std::make_shared<bg2e::scene::DrawableInstanceComponent>();
        const auto assigned = instance->setResource(*cache, handle);
        if (!assigned.ok())
        {
            throw std::runtime_error(assigned.message);
        }
        child->addComponent(instance);
        if (original->parent() && original->getComponent<bg2e::scene::TransformComponent>())
        {
            child->addComponent(original->getComponent<bg2e::scene::TransformComponent>()->clone());
        }
        else
        {
            child->addComponent(std::make_shared<bg2e::scene::TransformComponent>());
        }
        try
        {
            if (instance->submeshesCount() != owner->submeshesCount())
            {
                throw std::runtime_error("The owner's shared topology changed; retry instance creation after resource preparation");
            }
            // Cache creation uses immutable originals. This action instead captures
            // the selected owner's live appearance before admitting the insert.
            instance->setRayTracingEnabled(owner->rayTracingEnabled());
            for (uint32_t sub = 0; sub < owner->submeshesCount(); ++sub)
            {
                // Clone texture settings and retain generated sources, including
                // unsaved lightmaps, without sharing mutable material attributes.
                instance->material(sub) = bg2e::draw::cloneMaterialAttributes(owner->effectiveMaterial(sub));
                const auto local = owner->submeshTransform(sub);
                instance->setSubmeshTransform(local, sub);
                if (instance->localSubmeshTransform(sub) != local)
                {
                    throw std::invalid_argument("An owner submesh transform cannot be represented by an instance");
                }
                instance->setSubmeshName(owner->submeshName(sub), sub);
                instance->setSubmeshGroupName(owner->submeshGroupName(sub), sub);
                instance->setSubmeshVisibility(owner->submeshVisibility(sub), sub);
            }
            instance->prepareResources(cache->engine());
            if (!instance->resourcesLoaded())
            {
                throw std::runtime_error("Could not prepare the selected owner's instance appearance; no node was inserted");
            }
            _stage->prepareCommandCandidate(child);
            return std::make_unique<command::scene::EditNodes>(_stage, _selection,
                command::scene::NodeEdit::Insert, std::vector{child}, parent, "Create instance from owner",
                std::shared_ptr<bg2e::scene::Node>{}, true);
        }
        catch (...)
        {
            _stage->discardCandidate(child);
            throw;
        }
    });
}


void AppDelegate::rebindSelectedDrawableInstance(bg2e::draw::DrawableResourceHandle handle)
{
    const auto target = selectedNode(*this);
    queueDrawableCommand([this, handle, target = std::weak_ptr<bg2e::scene::Node>(target)]() -> std::unique_ptr<bg2e::app::Command>
    {
        const auto node = target.lock();
        auto* instance = node ? node->drawableInstance() : nullptr;
        const auto cache = handle.context();
        if (!node || !_stage->contains(node.get()) || !instance || !cache || &cache->engine() != _engine)
        {
            throw std::invalid_argument("Select a scene instance and a resident drawable from this document");
        }
        if (instance->resourceHandle() == handle)
        {
            return {};
        }
        auto replacement = std::make_shared<bg2e::scene::DrawableInstanceComponent>();
        const auto assigned = replacement->setResource(*cache, handle);
        if (!assigned.ok())
        {
            throw std::runtime_error(assigned.message);
        }
        replacement->setPriority(instance->priority());
        replacement->setIgnoreSerialization(instance->ignoreSerialization());
        return std::make_unique<command::component::EditMembership>(_stage, _selection, node,
            instance->shared_from_this(), replacement);
    });
}


void AppDelegate::replaceOwnerDrawable(std::weak_ptr<bg2e::scene::DrawableComponent> target)
try
{
    if (!sceneResourceOperationAvailable())
    {
        return;
    }
    const auto path = bg2e::app::FileDialog::getOpenFilePath({{"BG2 model", "bg2,vwglb"}});
    if (path.empty())
    {
        return;
    }
    queueDrawableCommand([this, target, path]() -> std::unique_ptr<bg2e::app::Command>
    {
        const auto owner = target.lock();
        if (!owner || !_stage->contains(owner->ownerNode()) || !owner->isSceneGeometry() || !owner->requiresBg2Export())
        {
            throw std::invalid_argument("Select a live drawable owner to replace its shared geometry");
        }
        auto replacement = bg2e::db::loadDrawableBg2(path, *_engine);
        return std::make_unique<command::scene::ReplaceDrawable>(_stage, _selection, owner, std::move(replacement));
    });
}
catch (const std::exception& error)
{
    bg2e::app::MessageBox::showError("Replace owner drawable", error.what());
}
catch (...)
{
    bg2e::app::MessageBox::showError("Replace owner drawable", "Could not prepare replacement; the owner is unchanged.");
}


void AppDelegate::removeSelectedDrawableInstance()
{
    const auto target = selectedNode(*this);
    queueDrawableCommand([this, target = std::weak_ptr<bg2e::scene::Node>(target)]() -> std::unique_ptr<bg2e::app::Command>
    {
        const auto node = target.lock();
        if (!node || !_stage->contains(node.get()) || !node->drawableInstance() || node == document()->root)
        {
            throw std::invalid_argument("Select a child scene instance to delete");
        }
        return std::make_unique<command::scene::EditNodes>(_stage, _selection,
            command::scene::NodeEdit::Remove, std::vector{node}, std::weak_ptr<bg2e::scene::Node>{}, "Delete drawable instance");
    });
}

}
