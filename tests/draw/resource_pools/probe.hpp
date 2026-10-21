/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <array>
#include <stdexcept>

namespace bg2e::draw::tests {
// Stopped initialized Engine. Allocates native resources and submits transitions;
// no shader dispatch or rendered-image claim. False means unavailable formats.
inline bool runTemporaryResourcePoolProbe(Engine& engine)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    if (!device.formatCapabilities(gpu::PixelFormat::R16_SFLOAT).supports(usage))
    {
        return false;
    }
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    gpu::ImageDescription description;
    description.size = {2,2};
    description.format = gpu::PixelFormat::R16_SFLOAT;
    description.usage = usage;
    gpu::PipelineLayoutDescription bindings;
    bindings.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
    bindings.resourceBindings.push_back({0,{1,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
    bindings.resourceBindings.push_back({0,{2,1},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
    auto layout = device.createPipelineLayout(bindings);
    TemporaryResourcePool pool(device);
    std::shared_ptr<gpu::Image> source, output;
    std::shared_ptr<TemporaryBindings> lease;
    const std::array<float,4> values{1,2,3,4};
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        source = pool.image(*command, description);
        output = pool.image(*command, description);
        require(source != output, "Two live image leases must never alias");
        lease = pool.bindings(*command, layout, 0, sizeof(values));
        lease->writeUniforms(values.data(), sizeof(values));
        lease->setUniformBuffer({0,1}, lease->uniforms().get());
        lease->setSampledImage({1,0}, source.get());
        lease->setStorageImage({2,1}, output.get());
        lease->update();
        command->transition(source.get(), gpu::ImageLayout::ShaderReadOnly);
        command->transition(output.get(), gpu::ImageLayout::General);
        auto concurrent = pool.bindings(*command, layout, 0, sizeof(values));
        require(concurrent->resources() != lease->resources() && concurrent->uniforms() != lease->uniforms(),
            "Same-frame invocations must own distinct sets and uniform buffers");
    });
    auto* originalSource = source.get();
    auto* originalOutput = output.get();
    auto* originalSet = lease->resources().get();
    auto* originalUniforms = lease->uniforms().get();
    output.reset();
    lease.reset();
    pool.collect();
    // Caller-held source remains occupied even though its writer completed.
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        auto recycled = pool.image(*command, description);
        require(recycled.get() == originalOutput && recycled != source,
            "Retired storage may be reused; externally held outputs must stay occupied");
        auto reused = pool.bindings(*command, layout, 0, sizeof(values));
        require(reused->resources().get() == originalSet && reused->uniforms().get() == originalUniforms,
            "Compatible retired invocation storage must survive warmup");
        reused->writeUniforms(values.data(), sizeof(values));
        reused->setUniformBuffer({0,1}, reused->uniforms().get());
        reused->setSampledImage({1,0}, source.get());
        reused->setStorageImage({2,1}, recycled.get());
        reused->update();
        command->transition(recycled.get(), gpu::ImageLayout::General);
    });
    pool.collect();

    // A later reader retains the source independently of the original writer.
    TemporaryResourcePool readers(device);
    auto reader = device.graphicsQueue().createCommandBuffer("pending pooled-image reader");
    reader->begin();
    auto reading = readers.bindings(*reader, layout, 0, sizeof(values));
    reading->setSampledImage({1,0}, source.get());
    reading.reset();
    source.reset();
    pool.collect();
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        auto candidate = pool.image(*command, description);
        require(candidate.get() != originalSource, "A later in-flight/unsubmitted reader must pin pooled input ownership");
    });
    auto readerTicket = reader->recordingCompletion();
    reader.reset();
    require(readerTicket->retired() && !readerTicket->completed(),
        "Abandoned work may retire storage but must never certify acceleration completion");
    readers.collect();
    pool.collect();
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        auto candidate = pool.image(*command, description);
        require(candidate.get() == originalSource, "Releasing every reader must make compatible storage reusable");
        auto resized = description;
        resized.size = {3,2};
        require(pool.image(*command, resized) != candidate, "Extent changes must invalidate image compatibility");
        auto otherLayout = device.createPipelineLayout(bindings);
        auto distinct = pool.bindings(*command, otherLayout, 0, sizeof(values));
        require(distinct->resources().get() != originalSet, "Native layout identity must distinguish compatible-looking layouts");
    });

    TemporaryPoolLimits small;
    small.images = 1;
    small.idleFrames = 1;
    TemporaryResourcePool bounded(device, small);
    std::array<std::weak_ptr<gpu::Image>,3> weak;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        for (auto& item : weak)
        {
            item = bounded.image(*command, description);
        }
    });
    bounded.collect();
    require(weak[0].expired() && weak[1].expired() && !weak[2].expired(),
        "Completed workload contraction must obey the retained-entry budget");
    bounded.collect();
    bounded.collect();
    require(weak[2].expired(), "Unused completed storage must age out");

    TemporaryResourcePool aborted(device);
    auto command = device.graphicsQueue().createCommandBuffer("abandoned pooled output");
    command->begin();
    auto image = aborted.image(*command, description);
    std::weak_ptr<gpu::Image> abandonedImage = image;
    command->transition(image.get(), gpu::ImageLayout::ShaderReadOnly);
    image.reset();
    aborted.collect();
    auto ticket = command->recordingCompletion();
    require(!ticket->retired(), "Live unsubmitted recordings must remain pinned");
    command.reset();
    aborted.collect();
    require(abandonedImage.expired(), "Abandoned images must leave the cache because recorded layout changes never executed");
    device.immediateSubmit([&](gpu::CommandBuffer* next)
    {
        auto fresh = aborted.image(*next, description);
        require(fresh->isValid(), "Abandoned recordings must recover with fresh image storage");
    });
    aborted.setEnabled(false);
    aborted.collect();
    device.immediateSubmit([&](gpu::CommandBuffer* next)
    {
        auto a = aborted.image(*next, description);
        auto b = aborted.image(*next, description);
        require(a != b, "Disabled pooling must allocate distinct fresh outputs");
    });
    return true;
}
}
