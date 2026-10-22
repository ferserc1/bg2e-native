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

#include <bg2e/draw/pipeline/TemporalSurfacePass.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <map>
#include <typeinfo>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) SurfaceUniforms {
    glm::uvec4 identity{0}; // canonical uint32 token, valid correspondence, reserved.
    glm::mat4 previousModel{1}, currentViewProjection{1}, previousViewProjection{1}, previousNormalFromCurrent{1};
    glm::vec4 extent{0};
};
static_assert(sizeof(SurfaceUniforms) == 288 && offsetof(SurfaceUniforms, identity) == 0);
static_assert(offsetof(SurfaceUniforms, previousModel) == 16);
static_assert(offsetof(SurfaceUniforms, currentViewProjection) == 80);
static_assert(offsetof(SurfaceUniforms, previousViewProjection) == 144);
static_assert(offsetof(SurfaceUniforms, previousNormalFromCurrent) == 208);
static_assert(offsetof(SurfaceUniforms, extent) == 272);
constexpr gpu::ShaderBinding SurfaceBinding{1,8}; // Vulkan set 2/binding 1, Metal fragment buffer 8.
constexpr gpu::ShaderBinding SurfaceVertexBinding{0,9};
const auto TokenUsage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled;
const auto DepthUsage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;

// Public pass callers receive the same conservative validation as renderer snapshots.
bool validSurfaceMatrix(const glm::mat4& matrix, bool affine)
{
    const float determinant = glm::determinant(matrix);
    if (!std::isfinite(determinant) || determinant == 0)
    {
        return false;
    }
    const auto inverse = glm::inverse(matrix);
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            if (!std::isfinite(matrix[c][r]) || !std::isfinite(inverse[c][r]))
            {
                return false;
            }
        }
    }
    return !affine || (matrix[0][3] == 0 && matrix[1][3] == 0 && matrix[2][3] == 0 && matrix[3][3] == 1);
}
}

TemporalSurfacePass::TemporalSurfacePass(std::filesystem::path shaderRoot, TemporaryPoolLimits limits)
    : _shaderRoot(shaderRoot.empty() ? base::PlatformTools::shaderPath() : std::move(shaderRoot)), _limits(limits)
{
}

void TemporalSurfacePass::initialize(Engine& engine, const RenderLayer& receiver, uint32_t frameSlots)
{
    if (_engine || !engine.device()->isValid())
    {
        throw std::logic_error("Temporal surface initialization requires a clean lifecycle and valid device");
    }
    const auto count = frameSlots ? frameSlots : engine.frameSlotCount();
    if (!count)
    {
        throw std::invalid_argument("Temporal surface initialization requires frame slots");
    }
    _engine = &engine;
    const auto& contract = receiver.contract();
    if (typeid(receiver) != typeid(GBuffer) || contract.shaderInterface != "draw.raster.gbuffer.v1" ||
        !contract.inputs.empty() || contract.outputs.size() != 7 || !gpu::isDepthFormat(contract.outputs[6].format))
    {
        _status = "Temporal surface unavailable: custom receiver or unsupported surface contract";
        return;
    }
    _depthFormat = contract.outputs[6].format;
    const auto expected = gbufferContract(_depthFormat);
    for (size_t i = 0; i < expected.outputs.size(); ++i)
    {
        const auto& a = contract.outputs[i];
        const auto& b = expected.outputs[i];
        if (a.semantic != b.semantic || a.format != b.format || a.extent != b.extent || a.type != b.type || a.usage != b.usage)
        {
            _status = "Temporal surface unavailable: receiver contract differs from built-in GBuffer";
            return;
        }
    }
    const auto limits = engine.device()->renderingCapabilities();
    const auto token = engine.device()->formatCapabilities(gpu::PixelFormat::R32_UINT);
    const auto depth = engine.device()->formatCapabilities(_depthFormat);
    uint32_t colorBytes = 0;
    bool formats = true;
    for (const auto format : {gpu::PixelFormat::R32_UINT,gpu::PixelFormat::R16G16_SFLOAT,
        gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R16G16B16A16_SFLOAT})
    {
        const auto capability = engine.device()->formatCapabilities(format);
        formats &= capability.supports(TokenUsage) && capability.colorAttachmentBytesPerSample != 0;
        colorBytes += capability.colorAttachmentBytesPerSample;
    }
    if (limits.maxColorAttachments < 4 || !limits.maxImageDimension2D || !formats || !token.supports(TokenUsage) ||
        !depth.supports(DepthUsage) || (limits.maxColorAttachmentBytesPerSample &&
        colorBytes > limits.maxColorAttachmentBytesPerSample))
    {
        _status = "Temporal surface unavailable: integer color/private sampled depth usages or combined attachment limits";
        return;
    }
    try
    {
        auto layout = drawBindingLayout();
        // Keep sets 0/1 exactly compatible with existing frame/object bindings.
        // Lighting/environment sets are unused; set 2 is dedicated to metadata.
        std::erase_if(layout.resourceBindings, [](const auto& binding)
        {
            return binding.set >= 2;
        });
        layout.resourceBindings.push_back({2,SurfaceVertexBinding,gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Vertex,1});
        layout.resourceBindings.push_back({2,SurfaceBinding,gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Fragment,1});
        layout.debugName = "draw.temporal_surface.v2";
        _layout = engine.device()->createPipelineLayout(layout);
        gpu::ShaderLib library(_shaderRoot,engine.backendType());
        _vertex = library.vertex("draw/temporal_surface/surface_v2",engine.device());
        _fragment = library.fragment("draw/temporal_surface/surface_v2",engine.device());
        gpu::GraphicsPipelineDescription description;
        description.vertexShader = _vertex.get();
        description.fragmentShader = _fragment.get();
        description.layout = _layout.get();
        description.depthFormat = _depthFormat;
        description.cullMode = gpu::CullMode::Back;
        description.depthState = gpu::DepthState{true,true,gpu::CompareOperation::LessOrEqual};
        for (const auto format : {gpu::PixelFormat::R32_UINT,gpu::PixelFormat::R16G16_SFLOAT,
            gpu::PixelFormat::R32G32B32A32_SFLOAT,gpu::PixelFormat::R16G16B16A16_SFLOAT})
        {
            description.colorAttachments.push_back({format,{}}); // Blending disabled.
        }
        description.addVertexBufferDescription(Drawable::DrawMesh::vertexBufferDescription());
        description.debugName = "draw temporal surface tokens";
        _pipeline = engine.device()->createGraphicsPipeline(description);
        _pool = std::make_shared<TemporaryResourcePool>(*engine.device(),_limits);
        _slots.resize(count);
        _status = "Available: built-in receiver replay, uint32 tokens, rigid motion, previous geometry and private depth";
    }
    catch (const std::exception& error)
    {
        _pipeline.reset();
        _layout.reset();
        _vertex.reset();
        _fragment.reset();
        _pool.reset();
        _slots.clear();
        _status = std::string("Temporal surface unavailable: ")+error.what();
    }
}

void TemporalSurfacePass::beginFrame(uint32_t index, uint64_t serial)
{
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Begin temporal surface slot once per acquired serial");
    }
    slot = {};
    slot.serial = serial;
    slot.prepared = true;
    _pool->collect();
}

TemporalSurfaceResult TemporalSurfacePass::record(const FrameContext& frame, const RasterFrame& raster,
    gpu::Size2D extent, std::span<const TemporalSurfaceInstance> instances,
    gpu::Image* opaqueDepth, uint64_t view, uint64_t layer, const TemporalSurfaceCamera& camera)
{
    if (!available())
    {
        return {};
    }
    if (&frame.engine != _engine || frame.commandBuffer.hasActiveScope() || !frame.commandBuffer.recordingCompletion() ||
        !raster.queue || !raster.frame || !raster.object || extent.isZero())
    {
        throw std::invalid_argument("Temporal surface recording requires acquired frame/queue/bindings and a receiver extent");
    }
    auto& slot = _slots.at(frame.frameSlot);
    if (!slot.prepared || slot.serial != frame.frameNumber)
    {
        throw std::logic_error("Begin the acquired temporal surface slot before recording");
    }
    for (const auto& invocation : slot.invocations)
    {
        if (invocation->view == view && invocation->layer == layer)
        {
            throw std::logic_error("Record each temporal receiver view/layer once per acquired frame");
        }
    }
    const auto limits = _engine->device()->renderingCapabilities();
    if (extent.width > limits.maxImageDimension2D || extent.height > limits.maxImageDimension2D)
    {
        throw std::invalid_argument("Temporal receiver extent exceeds device limits");
    }
    raster.queue->validate(frame.engine);
    if (bool(raster.item) != bool(opaqueDepth))
    {
        throw std::invalid_argument("Only single transparent receivers require preserved opaque depth");
    }
    if (opaqueDepth && (!opaqueDepth->isValid() || opaqueDepth->device() != _engine->device() || opaqueDepth->size() != extent ||
        opaqueDepth->pixelFormat() != _depthFormat || opaqueDepth->imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(opaqueDepth->usage(),gpu::ImageUsage::Sampled) || !gpu::hasFlag(opaqueDepth->usage(),gpu::ImageUsage::TransferSrc)))
    {
        throw std::invalid_argument("Preserved opaque depth must match the sampled/transfer-source receiver depth");
    }
    std::map<uint32_t,TemporalSurfaceInstance> metadata;
    for (const auto& instance : instances)
    {
        if (!instance.objectId || instance.objectId > raster.queue->items().size() ||
            (instance.correspondenceValid && !instance.token) || !metadata.emplace(instance.objectId,instance).second)
        {
            throw std::invalid_argument("Temporal surface instances require unique current object IDs and nonzero valid tokens");
        }
    }
    auto invocation = std::make_shared<Invocation>();
    invocation->view = view;
    invocation->layer = layer;
    if (raster.item)
    {
        const auto found = std::find_if(raster.queue->items().begin(),raster.queue->items().end(), [&](const auto& item)
        {
            return &item == raster.item;
        });
        // Sorted queues hold copies, so accept either current queue collection.
        const auto matches = [&](RenderQueueType type)
        {
            const auto& items = raster.queue->queue(type);
            return std::any_of(items.begin(),items.end(), [&](const auto& item)
            {
                return &item == raster.item;
            });
        };
        if (found == raster.queue->items().end() && !matches(RenderQueueType::Transparent) && !matches(RenderQueueType::SolidTransparent))
        {
            throw std::invalid_argument("Transparent temporal receiver must belong to the current extracted queue");
        }
        if ((raster.item->materialData.uvSets[1].z & MaterialTransparent) == 0)
        {
            throw std::invalid_argument("Single temporal receiver must use transparent GBuffer semantics");
        }
        invocation->items.push_back(*raster.item);
    }
    else
    {
        invocation->items = raster.queue->queue(RenderQueueType::Opaque);
    }
    auto* frameSet = raster.frame->set(frame);
    std::vector<gpu::ResourceSet*> objectSets;
    for (const auto& item : invocation->items)
    {
        objectSets.push_back(raster.object->set(frame,item.objectId));
        SurfaceUniforms data;
        const auto found = metadata.find(item.objectId);
        if (found != metadata.end())
        {
            data.identity.x = found->second.token;
            data.identity.y = found->second.correspondenceValid ? 1u : 0u;
        }
        const auto previous = found != metadata.end() && found->second.hasPreviousWorld ?
            found->second.previousWorld : item.worldMatrix;
        data.previousModel = previous;
        data.currentViewProjection = camera.currentViewProjection;
        data.previousViewProjection = camera.previousViewProjection;
        data.extent = glm::vec4(float(extent.width),float(extent.height),0,0);
        if (!validSurfaceMatrix(previous,true) || !validSurfaceMatrix(item.worldMatrix,true) ||
            glm::determinant(previous) < 0 || glm::determinant(item.worldMatrix) < 0 ||
            !validSurfaceMatrix(data.currentViewProjection,false) || !validSurfaceMatrix(data.previousViewProjection,false))
        {
            data.identity.y = 0;
            data.previousModel = item.worldMatrix;
            data.previousNormalFromCurrent = glm::mat4(1);
        }
        else
        {
            data.previousNormalFromCurrent = glm::transpose(glm::inverse(previous))*glm::transpose(item.worldMatrix);
        }
        if (data.identity.y && previous != item.worldMatrix)
        {
            invocation->result.legacyCompatible = false;
        }
        auto bindings = _pool->bindings(frame.commandBuffer,_layout,2,sizeof(data),{frame.frameSlot,view,layer,item.objectId});
        bindings->writeUniforms(&data,sizeof(data));
        bindings->setUniformBuffer(SurfaceVertexBinding,bindings->uniforms().get());
        bindings->setUniformBuffer(SurfaceBinding,bindings->uniforms().get());
        bindings->update();
        invocation->bindings.push_back(std::move(bindings));
    }
    gpu::ImageDescription description;
    description.size = extent;
    description.format = gpu::PixelFormat::R32_UINT;
    description.usage = TokenUsage;
    description.debugName = "draw temporal receiver token";
    invocation->result.token = _pool->image(frame.commandBuffer,description);
    for (auto [format, target] : {std::pair{gpu::PixelFormat::R16G16_SFLOAT,&invocation->result.motion},
        std::pair{gpu::PixelFormat::R32G32B32A32_SFLOAT,&invocation->result.previousPosition},
        std::pair{gpu::PixelFormat::R16G16B16A16_SFLOAT,&invocation->result.previousNormal}})
    {
        description.format = format;
        description.debugName = "draw temporal motion/previous geometry";
        *target = _pool->image(frame.commandBuffer,description);
    }
    description.format = _depthFormat;
    description.usage = DepthUsage;
    description.debugName = "draw temporal private receiver depth";
    invocation->result.depth = _pool->image(frame.commandBuffer,description);
    slot.invocations.push_back(invocation); // Retain resources even if subsequent recording fails.
    auto& command = frame.commandBuffer;
    if (opaqueDepth)
    {
        RenderTarget::copy(command,*opaqueDepth,*invocation->result.depth);
    }
    command.transition(invocation->result.token.get(),gpu::ImageLayout::ColorAttachment);
    command.transition(invocation->result.depth.get(),gpu::ImageLayout::DepthAttachment);
    gpu::RenderingDescription rendering;
    gpu::ColorAttachmentDescription color;
    color.image = invocation->result.token.get();
    color.load = gpu::AttachmentLoadOperation::Clear;
    color.clear.unsignedInteger = {0,0,0,0};
    rendering.colors.push_back(color);
    for (const auto& image : {invocation->result.motion,invocation->result.previousPosition,invocation->result.previousNormal})
    {
        command.transition(image.get(),gpu::ImageLayout::ColorAttachment);
        gpu::ColorAttachmentDescription attachment;
        attachment.image = image.get();
        attachment.load = gpu::AttachmentLoadOperation::Clear;
        attachment.clear.floatingPoint = {0,0,0,0};
        rendering.colors.push_back(attachment);
    }
    rendering.depth = gpu::DepthAttachmentDescription{.image = invocation->result.depth.get(),
        .load = opaqueDepth ? gpu::AttachmentLoadOperation::Load : gpu::AttachmentLoadOperation::Clear,
        .store = gpu::AttachmentStoreOperation::Store,.clearDepth = 1.0f};
    command.beginRendering(rendering); // Full viewport/clear; deliberately no regional scissor.
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(),0,frameSet);
    for (size_t i = 0; i < invocation->items.size(); ++i)
    {
        const auto& item = invocation->items[i];
        command.bindResourceSet(_pipeline.get(),1,objectSets[i]);
        command.bindResourceSet(_pipeline.get(),2,invocation->bindings[i]->resources().get());
        command.bindVertexBuffer(0,item.drawMesh->vertexBuffer());
        command.bindIndexBuffer(item.drawMesh->indexBuffer());
        item.drawMesh->drawSubmesh(&command,item.submeshIndex);
    }
    command.endRendering();
    command.transition(invocation->result.token.get(),gpu::ImageLayout::ShaderReadOnly);
    command.transition(invocation->result.depth.get(),gpu::ImageLayout::ShaderReadOnly);
    for (const auto& image : {invocation->result.motion,invocation->result.previousPosition,invocation->result.previousNormal})
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    return invocation->result;
}

bool TemporalSurfacePass::available() const
{
    return bool(_pipeline) && bool(_pool) && !_slots.empty();
}

const std::string& TemporalSurfacePass::status() const
{
    return _status;
}

void TemporalSurfacePass::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _pool.reset();
    _pipeline.reset();
    _layout.reset();
    _vertex.reset();
    _fragment.reset();
    _engine = nullptr;
    _depthFormat = gpu::PixelFormat::Undefined;
    _status = "Not initialized";
}
}
