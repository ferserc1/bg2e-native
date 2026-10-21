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

#include <bg2e/geo/modifiers.hpp>
#include <bg2e/utils/MaterialModifier.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/scene/TransformComponent.hpp>

#include <fstream>
#include <bg2e/db/scene.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <set>
#include <cmath>

namespace bg2e_composer {

void AppDelegate::notifyEdited(bg2e::draw::Renderer& renderer)
{
    invalidateLightmapSources();
    _stage->markModified();
    renderer.invalidateHistories();
    _stage->refreshCamera();
    renderer.attachScene(scene());
    setScene(scene()); // Rebind reflected catalogs at the next stopped boundary too.
    requestSceneFrame();
}

void AppDelegate::queueNodeEdit(std::weak_ptr<bg2e::scene::Node> target,
    std::function<void(bg2e::scene::Node&)> operation, bool rebuild)
{
    if (!document() || !document()->root)
    {
        return;
    }
    _status.clear();
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), target, generation, operation = std::move(operation), rebuild](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        auto node = target.lock();
        if (!node || !document() || document()->generation != generation ||
            (!_stage->contains(node.get()) && node != _stage->fillLightNode()))
        {
            return;
        }
        invalidateLightmapSources();
        try
        {
            _engine->device()->waitIdle();
            operation(*node);
            if (rebuild)
            {
                _stage->prepareTools();
                _selection->refreshSelection();
            }
        }
        catch (const std::exception& error)
        {
            _status = error.what();
        }
        // Edits/texture callbacks can fail after modifying part of the target.
        notifyEdited(renderer);
    });
}

void AppDelegate::selectNode(std::weak_ptr<bg2e::scene::Node> target, uint32_t submesh, bool additive)
{
    const auto generation = document() ? document()->generation : 0;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), target, submesh, additive, generation](auto&)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        auto node = target.lock();
        if (!node || !document() || document()->generation != generation || !_stage->contains(node.get()))
        {
            return;
        }
        if (!additive)
        {
            _selection->deselect();
        }
        if (auto component = node->drawable())
        {
            if (_selection->isSelected(node.get(), component, submesh))
            {
                _selection->removeFromSelectedItems(node.get(), submesh);
            }
            else
            {
                _selection->addToSelectedItems(node.get(), component, submesh);
            }
        }
        else
        {
            _selection->addToSelectedItems(node.get());
        }
    });
}

void AppDelegate::editGeometry(unsigned operation)
{
    if (!_selection)
    {
        return;
    }
    std::set<bg2e::draw::Drawable*> visited;
    for (const auto& item : _selection->selectedItems())
    {
        auto drawable = item->lockDrawMesh();
        if (!drawable || !visited.insert(drawable.get()).second)
        {
            continue;
        }
        const auto identity = std::weak_ptr<bg2e::draw::Drawable>(drawable);
        queueNodeEdit(item->node, [identity, operation](auto& node)
        {
            using namespace bg2e::geo;
            auto drawable = identity.lock();
            if (!drawable || !node.drawable() || node.drawable()->drawDrawable() != drawable)
            {
                return;
            }
            // Modifiers are borrowed by Drawable::applyModifier, never transferred.
            if (operation == 0)
            {
                ConvertAxisModifier<Mesh> modifier(ConvertAxisModifier<Mesh>::Mode::ZtoY);
                drawable->applyModifier(&modifier);
            }
            else if (operation == 1)
            {
                CenterGeometryModifier<Mesh> modifier;
                drawable->applyModifier(&modifier);
            }
            else if (operation == 2)
            {
                PutOnFloorGeometryModifier<Mesh> modifier;
                drawable->applyModifier(&modifier);
            }
            else if (operation == 3)
            {
                ApplyTransformModifier<Mesh> modifier(glm::scale(glm::mat4(1), glm::vec3(0.01f)));
                drawable->applyModifier(&modifier);
            }
        }, true);
    }
}

void AppDelegate::duplicateSelection()
{
    if (auto node = _selection ? _selection->selectedNode() : nullptr)
    {
        queueNodeEdit(node->shared_from_this(), [this](auto& target)
        {
            _stage->duplicate(target.shared_from_this());
        }, true);
    }
}

void AppDelegate::copyMaterial()
{
    const auto item = _selection ? _selection->selectedItem() : nullptr;
    if (!item)
    {
        return;
    }
    const auto target = *item;
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), target, generation](auto&)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        auto node = target.node.lock();
        auto mesh = target.lockDrawMesh();
        if (document()->generation == generation && node && _stage->contains(node.get()) && mesh)
        {
            try
            {
                _clipboard = mesh->drawMaterial(target.submesh)->clone();
                _status = "Material copied";
            }
            catch (const std::exception& error)
            {
                _status = error.what();
            }
        }
    });
}

void AppDelegate::pasteMaterial()
{
    if (!_clipboard || !_selection)
    {
        return;
    }
    for (const auto& item : _selection->selectedItems())
    {
        auto target = *item;
        queueNodeEdit(target.node, [target, snapshot = _clipboard](auto&)
        {
            auto mesh = target.lockDrawMesh();
            if (!mesh)
            {
                return;
            }
            auto material = mesh->drawMaterial(target.submesh);
            material->setMaterialAttributes(bg2e::draw::cloneMaterialAttributes(snapshot->materialAttributes()));
            for (uint32_t slot = 0; slot < 6; ++slot)
            {
                const auto kind = static_cast<bg2e::draw::TextureSlot>(slot);
                material->setTexture(kind, snapshot->texture(kind)->clone());
            }
            mesh->materialDefinition(target.submesh) = bg2e::draw::cloneMaterialAttributes(material->materialAttributes());
        });
    }
}

void AppDelegate::removeSelection()
{
    if (auto node = _selection ? _selection->selectedNode() : nullptr)
    {
        queueNodeEdit(node->shared_from_this(), [this](auto& target)
        {
            _selection->deselect();
            _stage->remove(target.shared_from_this());
        }, true);
    }
}

void AppDelegate::applyMaterialPatch(const std::filesystem::path& path)
{
    if (!_selection || path.empty())
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
        for (const auto& item : _selection->selectedItems())
        {
            const auto identity = item->drawMesh;
            const auto submesh = item->submesh;
            queueNodeEdit(item->node, [identity, submesh, patch](auto& node)
            {
                auto mesh = identity.lock();
                if (mesh && node.drawable() && node.drawable()->drawDrawable() == mesh && submesh < mesh->submeshesCount())
                {
                    patch.apply(*mesh, submesh);
                }
            });
        }
    }
    catch (const std::exception& error)
    {
        _status = error.what();
    }
}

void AppDelegate::setFillLight(bool enabled)
{
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), enabled](auto& renderer)
    {
        if (auto owner = weak.lock())
        {
            owner->_stage->setFillLight(enabled);
            renderer.invalidateHistories();
        }
    });
}

void AppDelegate::setFloor(bool visible, float height)
{
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), visible, height](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        _stage->setFloor(visible, height);
        renderer.invalidateHistories();
    });
}

void AppDelegate::editEnvironment(std::function<void(bg2e::scene::EnvironmentComponent&)> operation)
{
    if (!document() || !document()->root)
    {
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), generation, operation = std::move(operation)](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner || !owner->document() || owner->document()->generation != generation)
        {
            return;
        }
        bool adopted = false;
        try
        {
            auto candidate = owner->_stage->environmentPreset();
            operation(*candidate->environment());
            auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(&renderer);
            if (!deferred || !deferred->environmentGenerator())
            {
                throw std::logic_error("The renderer has no environment generation service");
            }
            owner->invalidateLightmapSources();
            // A failed generator preserves its previously published GPU snapshot.
            deferred->environmentGenerator()->synchronize(candidate->environment());
            owner->_stage->adoptEnvironment(candidate);
            adopted = true;
            owner->_stage->prepareTools();
            owner->notifyEdited(renderer);
            owner->_status.clear();
        }
        catch (const std::exception& error)
        {
            if (adopted)
            {
                owner->_stage->markModified();
                renderer.invalidateHistories();
                owner->requestSceneFrame();
            }
            owner->_status = error.what();
        }
    });
}

void AppDelegate::environmentPreset(const std::filesystem::path& path, bool save)
{
    if (!document() || !document()->root || path.empty())
    {
        return;
    }
    const auto generation = document()->generation;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), path, save, generation](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner || !document() || document()->generation != generation)
        {
            return;
        }
        bool adopted = false;
        try
        {
            if (save)
            {
                auto preset = _stage->environmentPreset();
                _status = bg2e::db::saveScene(preset.get(), *_engine, path) ?
                    "Environment preset saved" : "Environment preset save failed (see log)";
            }
            else
            {
                auto loaded = bg2e::db::loadScene(path); // Graphics-free schema validation first.
                const auto& children = loaded->rootNode()->children();
                if (children.size() != 1)
                {
                    throw std::invalid_argument("Environment preset requires exactly one root");
                }
                _stage->validateEnvironmentPreset(children[0]);
                auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(&renderer);
                if (!deferred || !deferred->environmentGenerator())
                {
                    throw std::logic_error("The renderer has no environment generation service");
                }
                owner->invalidateLightmapSources();
                deferred->environmentGenerator()->synchronize(children[0]->environment());
                _stage->restoreEnvironment(children[0]);
                adopted = true;
                _stage->prepareTools();
                _selection->refreshSelection();
                notifyEdited(renderer);
                _status = "Environment preset restored";
            }
        }
        catch (const std::exception& error)
        {
            if (adopted)
            {
                _stage->markModified();
                renderer.invalidateHistories();
                requestSceneFrame();
            }
            _status = error.what();
        }
    });
}

void AppDelegate::setCameraIntrinsics(float fov, float nearPlane, float farPlane)
{
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), fov, nearPlane, farPlane](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner || !std::isfinite(fov) || !std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
            fov <= 0 || fov >= 179 || nearPlane <= 0 || farPlane <= nearPlane)
        {
            return;
        }
        auto* camera = _stage->camera();
        if (auto* projection = camera->projection<bg2e::math::PerspectiveProjection>())
        {
            projection->setFov(fov);
        }
        else if (auto* projection = camera->projection<bg2e::math::OpticalProjection>())
        {
            projection->setFocalLength(projection->frameSize() / (2.0f * std::tan(glm::radians(fov) * 0.5f)));
        }
        else
        {
            return;
        }
        camera->projection()->setNear(nearPlane);
        camera->projection()->setFar(farPlane);
        notifyEdited(renderer);
    });
}

void AppDelegate::setHighlight(float intensity)
{
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), intensity](auto&)
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
