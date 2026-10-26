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

#include <bg2e/manipulation/GizmoAndSelectionRenderer.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/gpu/Device.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace bg2e::manipulation {

void GizmoAndSelectionRenderer::initialize(draw::Engine& engine, const draw::pipeline::OverlayConfiguration& configuration)
{
    if (_overlayInitialized || (_drawEngine && _drawEngine != &engine))
    {
        throw std::logic_error("Overlay lifecycle cannot be shared or change engine");
    }
    _drawEngine = &engine;
    if (!_graphics)
    {
        _graphics = std::make_shared<draw::pipeline::MeshOverlayPass>();
    }
    // Mark before initialization so cleanup covers partial graphics failure.
    _overlayInitialized = true;
    _graphics->initialize(engine, configuration);
    _overlayFrames.resize(configuration.frameSlots);
}

std::shared_ptr<draw::pipeline::MeshOverlayPass> GizmoAndSelectionRenderer::graphics() const
{
    return _graphics;
}

void GizmoAndSelectionRenderer::attachScene(std::shared_ptr<scene::Node> root)
{
    if (!_overlayInitialized)
    {
        throw std::logic_error("Initialize draw overlay before scene attachment");
    }
    _drawEngine->device()->waitIdleForResourceUpdate();
    _drawScene = std::move(root);
    _edgeMeshes.clear();
    _drawNodes.clear();
    for (auto& slot : _overlayFrames)
    {
        slot = {};
    }
}

void GizmoAndSelectionRenderer::resize(const draw::pipeline::OverlayConfiguration& configuration)
{
    if (!_overlayInitialized)
    {
        throw std::logic_error("Initialize draw overlay before resizing");
    }
    _graphics->resize(configuration);
    _overlayFrames.clear();
    _overlayFrames.resize(configuration.frameSlots);
}

void GizmoAndSelectionRenderer::collectDrawNodes(scene::Node* root)
{
    _drawNodes.clear();
    if (!root)
    {
        return;
    }
    for (auto* node = root; node; node = node->parent())
    {
        if (node->disabled())
        {
            return;
        }
    }
    // Preserve the visitor filtering configuration.
    const bool ignored = ignoreDisabled();
    setIgnoreDisabled(true);
    try
    {
        root->accept(this);
    }
    catch (...)
    {
        setIgnoreDisabled(ignored);
        _drawNodes.clear();
        throw;
    }
    setIgnoreDisabled(ignored);
}

bool GizmoAndSelectionRenderer::cacheLines(const std::shared_ptr<draw::Drawable>& drawable)
{
    if (!drawable || !drawable->isLoaded())
    {
        return false;
    }
    if (drawable->engine() != _drawEngine)
    {
        throw std::invalid_argument("Overlay resource belongs to another draw engine");
    }
    auto source = drawable->drawMesh();
    if (!source)
    {
        throw std::logic_error("Loaded overlay geometry needs a GPU mesh");
    }
    if (std::any_of(_edgeMeshes.begin(), _edgeMeshes.end(), [&](const EdgeMesh& entry)
    {
        return entry.source.lock() == source;
    }))
    {
        return false;
    }
    // Explicit edge indices avoid optional polygon/wide-line backend features.
    const auto& triangles = source->meshData();
    draw::Drawable::Mesh edges;
    edges.vertices = triangles.vertices;
    for (const auto& submesh : triangles.submeshes)
    {
        if (submesh.indexCount % 3 || size_t(submesh.firstIndex) + submesh.indexCount > triangles.indices.size())
        {
            throw std::invalid_argument("Overlay wireframe needs triangle-list submeshes");
        }
        std::set<std::pair<uint32_t, uint32_t>> unique;
        for (size_t i = submesh.firstIndex; i < size_t(submesh.firstIndex) + submesh.indexCount; i += 3)
        {
            for (size_t edge = 0; edge < 3; ++edge)
            {
                auto a = triangles.indices[i + edge];
                auto b = triangles.indices[i + (edge + 1) % 3];
                if (a >= edges.vertices.size() || b >= edges.vertices.size())
                {
                    throw std::out_of_range("Overlay edge references a missing vertex");
                }
                if (a != b)
                {
                    unique.emplace(std::min(a, b), std::max(a, b));
                }
            }
        }
        const size_t first = edges.indices.size();
        if (unique.size() > (std::numeric_limits<uint32_t>::max() - first) / 2)
        {
            throw std::length_error("Overlay edge indices exceed uint32 ranges");
        }
        for (const auto& edge : unique)
        {
            edges.indices.push_back(edge.first);
            edges.indices.push_back(edge.second);
        }
        edges.submeshes.push_back({uint32_t(first), uint32_t(edges.indices.size() - first)});
    }
    std::shared_ptr<draw::Drawable::DrawMesh> lines;
    if (!edges.indices.empty())
    {
        lines = std::make_shared<draw::Drawable::DrawMesh>();
        lines->setMeshData(edges);
        lines->build(_drawEngine->device());
    }
    _edgeMeshes.push_back({source, std::move(lines)});
    return true;
}

std::shared_ptr<draw::Drawable::DrawMesh> GizmoAndSelectionRenderer::lineMesh(const std::shared_ptr<draw::Drawable::DrawMesh>& source) const
{
    for (const auto& entry : _edgeMeshes)
    {
        if (entry.source.lock() == source)
        {
            return entry.lines;
        }
    }
    throw std::logic_error("Prepare overlay edge resources before acquired-frame update/record");
}

bool GizmoAndSelectionRenderer::prepareFrame()
{
    if (!_overlayInitialized)
    {
        throw std::logic_error("Initialize draw overlay before preparing resources");
    }
    collectDrawNodes(_drawScene.get());
    bool changed = false;
    std::erase_if(_edgeMeshes, [](const EdgeMesh& entry)
    {
        return entry.source.expired();
    });
    for (const auto& weak : _drawNodes)
    {
        auto node = weak.lock();
        if (!node)
        {
            continue;
        }
        if (auto* component = node->drawable())
        {
            changed = cacheLines(component->drawable()) || changed;
        }
        if (auto* gizmo = node->getComponent<GizmoComponent>())
        {
            gizmo->synchronizeResources();
            changed = cacheLines(gizmo->drawable()) || changed;
            changed = cacheLines(gizmo->transformDrawable()) || changed;
        }
    }
    return changed;
}

void GizmoAndSelectionRenderer::update(const draw::pipeline::OverlayContext& context)
{
    const auto& frame = context.frame;
    if (!_overlayInitialized || &frame.engine != _drawEngine || frame.frameSlot >= _overlayFrames.size() ||
        context.sceneRoot != _drawScene.get() || context.queue.engine() != _drawEngine || frame.commandBuffer.hasActiveScope())
    {
        throw std::invalid_argument("Overlay update needs its current scene, engine and acquired slot");
    }
    if (!std::isfinite(_lineIntensity))
    {
        throw std::invalid_argument("Selection line intensity must be finite");
    }
    auto& slot = _overlayFrames[frame.frameSlot];
    if (slot.prepared && slot.serial == frame.frameNumber)
    {
        throw std::logic_error("Overlay updates once per acquired frame");
    }
    draw::pipeline::OverlayDrawList draws;
    const auto& settings = _graphics->settings();
    for (const auto& item : context.queue.items())
    {
        draws.occluders.push_back({item.drawMesh, item.submeshIndex, item.worldMatrix, {1, 1, 1, 1}});
        auto node = item.node.lock();
        auto* selectable = node ? node->getComponent<SelectableComponent>() : nullptr;
        if (!selectable)
        {
            continue;
        }
        selectable->update(0);
        if (!selectable->isSelected(item.submeshIndex))
        {
            continue;
        }
        auto mesh = settings.highlightLines ? lineMesh(item.drawMesh) : item.drawMesh;
        if (mesh && mesh->meshData().submeshes.at(item.submeshIndex).indexCount)
        {
            const auto color = item.drawMaterial->materialAttributes().albedo();
            draws.highlights.push_back({mesh, item.submeshIndex, item.worldMatrix,
                {1.0f - color.r, 1.0f - color.g, 1.0f - color.b, std::clamp(_lineIntensity, 0.0f, 1.0f)}});
        }
    }
    collectDrawNodes(context.sceneRoot);
    for (const auto& weak : _drawNodes)
    {
        auto node = weak.lock();
        auto* gizmo = node ? node->getComponent<GizmoComponent>() : nullptr;
        if (!gizmo)
        {
            continue;
        }

        const auto append = [&](const std::shared_ptr<draw::Drawable>& resource, bool transform)
        {
            if (!resource || !resource->isLoaded())
            {
                return;
            }
            if (resource->engine() != _drawEngine)
            {
                throw std::invalid_argument("Overlay gizmo belongs to another engine");
            }
            const auto world = transform ? gizmo->renderTransform(node->worldMatrix(), context.view, context.projection, GizmoType::Transform) :
                gizmo->renderTransform(node->worldMatrix(), context.view, context.projection);
            auto mesh = !transform && settings.typeGizmoLines ? lineMesh(resource->drawMesh()) : resource->drawMesh();
            if (!mesh)
            {
                return;
            }
            for (uint32_t i = 0; i < resource->submeshesCount(); ++i)
            {
                if (!resource->submeshVisibility(i) || !resource->drawMaterial(i)->materialAttributes().visible() ||
                    (transform && !GizmoComponent::isTransformSubmeshVisible(resource->submeshName(i), resource->submeshGroupName(i))))
                {
                    continue;
                }
                const auto color = transform ? resource->drawMaterial(i)->materialAttributes().albedo() : gizmo->gizmoColor();
                const auto type = transform ? GizmoType::Transform : gizmo->currentGizmoType();
                const float alpha = GizmoComponent::gizmoOpacity(type);
                auto& list = transform ? draws.transformGizmos : draws.typeGizmos;
                list.push_back({mesh, i, world * resource->submeshTransform(i), {color.r, color.g, color.b, alpha}});
            }
        };
        if (gizmo->currentGizmoType() != GizmoType::None && GizmoComponent::isGizmoVisible(gizmo->currentGizmoType()))
        {
            append(gizmo->drawable(), false);
        }
        if (gizmo->transformVisible() && node->transform() && GizmoComponent::isGizmoVisible(GizmoType::Transform))
        {
            append(gizmo->transformDrawable(), true);
        }
    }
    slot.draws = std::move(draws);
    slot.serial = frame.frameNumber;
    slot.prepared = true;
}

void GizmoAndSelectionRenderer::record(const draw::pipeline::OverlayContext& context)
{
    const auto& frame = context.frame;
    if (!_overlayInitialized || &frame.engine != _drawEngine || frame.frameSlot >= _overlayFrames.size())
    {
        throw std::invalid_argument("Overlay recording needs its initialized acquired slot");
    }
    auto& slot = _overlayFrames[frame.frameSlot];
    if (!slot.prepared || slot.serial != frame.frameNumber || context.sceneRoot != _drawScene.get())
    {
        throw std::logic_error("Update the overlay snapshot before recording");
    }
    _graphics->record(context, slot.draws);
    slot.prepared = false;
}

void GizmoAndSelectionRenderer::cleanup()
{
    if (_overlayInitialized)
    {
        _graphics->cleanup();
    }
    _overlayFrames.clear();
    _edgeMeshes.clear();
    _drawNodes.clear();
    _drawScene.reset();
    _overlayInitialized = false;
    // Retain the borrowed engine for same-engine reuse.
}

}
