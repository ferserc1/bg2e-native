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

#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <cmath>
#include <limits>

namespace bg2e::manipulation {

void SelectionManager::setPickPixelScale(float x, float y)
{
    if (!std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0)
    {
        throw std::invalid_argument("Pick pixel scale must be finite and positive");
    }
    _pickPixelScale = {x, y};
}

bool SelectionManager::pickDrawObject(scene::Node* root, const glm::mat4& view,
    const glm::mat4& projection, const math::Viewport& viewport,
    uint32_t x, uint32_t y, SelectionItem* result)
{
    if (!root)
    {
        return false;
    }
    for (auto* node = root; node; node = node->parent())
    {
        if (node->disabled())
        {
            return false;
        }
    }
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!std::isfinite(view[column][row]) || !std::isfinite(projection[column][row]))
            {
                throw std::invalid_argument("Pick matrices must be finite and unjittered");
            }
        }
    }
    for (float v : {viewport.x, viewport.y, viewport.width, viewport.height})
    {
        if (!std::isfinite(v))
        {
            throw std::invalid_argument("Pick viewport must be finite");
        }
    }
    if (viewport.width <= 0 || viewport.height <= 0 ||
        double(x) < viewport.x || double(y) < viewport.y ||
        double(x) >= double(viewport.x) + viewport.width ||
        double(y) >= double(viewport.y) + viewport.height)
    {
        return false;
    }
    const double width = std::ceil(double(viewport.width) * _pickPixelScale.x);
    const double height = std::ceil(double(viewport.height) * _pickPixelScale.y);
    if (width > std::numeric_limits<uint32_t>::max() || height > std::numeric_limits<uint32_t>::max())
    {
        throw std::out_of_range("Pick extent exceeds GPU dimensions");
    }
    const gpu::Size2D extent{uint32_t(width), uint32_t(height)};
    const auto px = uint32_t(std::floor((double(x) - viewport.x) * _pickPixelScale.x));
    const auto py = uint32_t(std::floor((double(y) - viewport.y) * _pickPixelScale.y));
    auto* device = _drawEngine->device();
    if (!device || !device->isValid())
    {
        throw std::logic_error("Draw picking requires a live device");
    }
    // Caller has stopped frame/scene producers; drain before taking the snapshot.
    device->waitIdle();
    draw::RenderQueue queue;
    draw::RenderQueueVisitor extractor;
    extractor.enqueue(root, *_drawEngine, queue);

    struct Entry {
        SelectionItem selection;
        std::shared_ptr<draw::Drawable::DrawMesh> mesh;
        glm::mat4 world{1};
        uint32_t id = 0;
        std::weak_ptr<GizmoComponent> gizmo;
    };
    std::vector<Entry> entries;
    for (const auto& item : queue.items())
    {
        auto node = item.node.lock();
        auto* component = node ? node->drawable() : nullptr;
        auto* selectable = node ? node->getComponent<SelectableComponent>() : nullptr;
        if (!component || !selectable)
        {
            continue;
        }
        selectable->update(0);
        SelectionItem selection;
        selection.node = node;
        selection.drawable = std::dynamic_pointer_cast<scene::DrawableComponent>(component->shared_from_this());
        selection.mesh = component->drawable();
        selection.submesh = item.submeshIndex;
        selection.selectionIdentifier = selectable->identifier(item.submeshIndex);
        entries.push_back({selection, item.drawMesh, item.worldMatrix, item.objectId, {}});
    }
    const size_t geometryEnd = entries.size();
    // A local visitor only collects tool owners; scene geometry uses the shared extractor.
    struct ToolVisitor : scene::NodeVisitor {
        std::vector<std::shared_ptr<scene::Node>> nodes;

        void visit(scene::Node* node) override
        {
            if (node->getComponent<GizmoComponent>())
            {
                nodes.push_back(node->shared_from_this());
            }
        }
    } tools;
    tools.setIgnoreDisabled(true);
    root->accept(&tools);
    uint32_t nextId = uint32_t(queue.items().size()) + 1;
    const auto appendTool = [&](const std::shared_ptr<scene::Node>& node, PickKind kind)
    {
        auto* gizmo = node->getComponent<GizmoComponent>();
        if (!gizmo)
        {
            return;
        }
        const bool transform = kind == PickKind::TransformGizmo;
        auto resource = transform ? gizmo->transformDrawable() : gizmo->drawable();
        if (!resource || !resource->isLoaded())
        {
            return;
        }
        if (resource->engine() != _drawEngine)
        {
            throw std::invalid_argument("Pick gizmo belongs to another draw engine");
        }
        const auto world = transform ? gizmo->renderTransform(node->worldMatrix(), view, projection, GizmoType::Transform) :
            gizmo->renderTransform(node->worldMatrix(), view, projection);
        for (uint32_t i = 0; i < resource->submeshesCount(); ++i)
        {
            if (transform && !GizmoComponent::isTransformSubmeshVisible(resource->submeshName(i), resource->submeshGroupName(i)))
            {
                continue;
            }
            if (nextId > 0x00ffffffu)
            {
                throw std::length_error("Picking exceeds the RGBA8 24-bit ID range");
            }
            SelectionItem selection;
            selection.node = node;
            selection.kind = kind;
            selection.submesh = i;
            entries.push_back({selection, resource->drawMesh(), world * resource->submeshTransform(i), nextId++,
                std::dynamic_pointer_cast<GizmoComponent>(gizmo->shared_from_this())});
        }
    };
    for (const auto& node : tools.nodes)
    {
        auto* gizmo = node->getComponent<GizmoComponent>();
        if (node->getComponent<SelectableComponent>() && gizmo->currentGizmoType() != GizmoType::None &&
            GizmoComponent::isGizmoVisible(gizmo->currentGizmoType()))
        {
            appendTool(node, PickKind::Gizmo);
        }
    }
    const size_t toolsEnd = entries.size();
    if (_transformManipulationEnabled && GizmoComponent::isGizmoVisible(GizmoType::Transform))
    {
        for (const auto& node : tools.nodes)
        {
            if (auto* gizmo = node->getComponent<GizmoComponent>(); gizmo->transformVisible() && node->transform())
            {
                appendTool(node, PickKind::TransformGizmo);
            }
        }
    }
    gpu::ImageDescription imageDescription;
    imageDescription.size = extent;
    imageDescription.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    imageDescription.usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    auto color = device->createImage(imageDescription);
    imageDescription.format = gpu::PixelFormat::D32_SFLOAT;
    imageDescription.usage = gpu::ImageUsage::DepthStencil;
    auto depth = device->createImage(imageDescription);
    auto transformDepth = toolsEnd < entries.size() ? device->createImage(imageDescription) : nullptr;
    // An explicit vec4 keeps C++, GLSL and Metal push layouts identical (80 bytes).
    struct Push {
        glm::mat4 mvp;
        glm::uvec4 identifier;
    };
    static_assert(sizeof(Push) == 80);
    gpu::PipelineLayoutDescription layoutDescription;
    layoutDescription.pushConstants.push_back({0, sizeof(Push), gpu::ShaderStage::Vertex});
    auto layout = device->createPipelineLayout(layoutDescription);
    gpu::ShaderLib shaders(base::PlatformTools::shaderPath(), _drawEngine->backendType());
    auto vertex = shaders.vertex("draw/picking/picking", device);
    auto fragment = shaders.fragment("draw/picking/picking", device);
    gpu::GraphicsPipelineDescription pipelineDescription;
    pipelineDescription.vertexShader = vertex.get();
    pipelineDescription.fragmentShader = fragment.get();
    pipelineDescription.layout = layout.get();
    pipelineDescription.colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
    pipelineDescription.depthFormat = gpu::PixelFormat::D32_SFLOAT;
    pipelineDescription.cullMode = gpu::CullMode::Back;
    pipelineDescription.depthState = gpu::DepthState{true, true, gpu::CompareOperation::LessOrEqual};
    pipelineDescription.addVertexBufferDescription(draw::Drawable::DrawMesh::vertexBufferDescription());
    auto pipeline = device->createGraphicsPipeline(pipelineDescription);
    pipelineDescription.depthState = gpu::DepthState{false, false, gpu::CompareOperation::Always};
    auto topPipeline = device->createGraphicsPipeline(pipelineDescription);
    device->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        command->transition(color.get(), gpu::ImageLayout::ColorAttachment);
        command->transition(depth.get(), gpu::ImageLayout::DepthAttachment);
        gpu::RenderingDescription rendering;
        rendering.colors.push_back({.image = color.get(), .load = gpu::AttachmentLoadOperation::Clear});
        rendering.depth = gpu::DepthAttachmentDescription{.image = depth.get(), .load = gpu::AttachmentLoadOperation::Clear};
        command->beginRendering(rendering);
        command->clearColor(0, {0, 0, 0, 0});
        command->clearDepth(1.0f);
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (i == toolsEnd && toolsEnd < entries.size())
            {
                command->endRendering();
                // Explicit color dependency across dynamic rendering scopes. The
                // transform pass owns fresh depth, avoiding a depth write hazard.
                command->transition(color.get(), gpu::ImageLayout::TransferSrc);
                command->transition(color.get(), gpu::ImageLayout::ColorAttachment);
                command->transition(transformDepth.get(), gpu::ImageLayout::DepthAttachment);
                rendering.colors.front().load = gpu::AttachmentLoadOperation::Load;
                rendering.depth->image = transformDepth.get();
                rendering.depth->load = gpu::AttachmentLoadOperation::Clear;
                command->beginRendering(rendering);
            }
            auto& entry = entries[i];
            command->bindPipeline(i >= geometryEnd && i < toolsEnd ? topPipeline.get() : pipeline.get());
            Push push{projection * view * entry.world, {entry.id, 0, 0, 0}};
            command->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(push), &push);
            command->bindVertexBuffer(0, entry.mesh->vertexBuffer());
            command->bindIndexBuffer(entry.mesh->indexBuffer());
            entry.mesh->drawSubmesh(command, entry.selection.submesh);
        }
        command->endRendering();
        command->transition(color.get(), gpu::ImageLayout::TransferSrc);
    });
    std::vector<uint8_t> pixels;
    color->readPixelsRGBA8(pixels, gpu::ImageLayout::TransferSrc);
    const size_t offset = (size_t(py) * extent.width + px) * 4;
    if (offset + 3 >= pixels.size())
    {
        throw std::runtime_error("Incomplete pick image readback");
    }
    const uint32_t id = uint32_t(pixels[offset]) | (uint32_t(pixels[offset + 1]) << 8) | (uint32_t(pixels[offset + 2]) << 16);
    for (const auto& entry : entries)
    {
        if (entry.id != id || entry.selection.expired())
        {
            continue;
        }
        if (entry.selection.kind != PickKind::Drawable)
        {
            auto node = entry.selection.node.lock();
            auto gizmo = entry.gizmo.lock();
            if (!node || !gizmo || node->getComponent<GizmoComponent>() != gizmo.get())
            {
                return false;
            }
        }
        *result = entry.selection;
        return true;
    }
    return false;
}

}
