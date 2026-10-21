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

#pragma once

#include <bg2e/draw/all.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/scene/Node.hpp>
#include <algorithm>
#include <stdexcept>
#if BG2E_IS_MAC
#include <bg2e/gpu/metal/ResourceSet.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#endif

namespace bg2e::draw::tests {
// Stopped initialized engine, no active/background frame. False is an unavailable
// backend/profile, not a native-validation pass. Builds TLAS, but does not dispatch.
inline bool runShadowBindingProbe(Engine& engine)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto available = device.resourceArrayCapabilities().profileCapacity(2,1,10);
    if (!device.rayTracingCapabilities().accelerationStructure || available < 2)
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
    const auto capacity = std::min(available, 9u);
    const auto description = softShadowBindingLayout(capacity);
    for (const auto& binding : description.resourceBindings)
    {
        const bool stable = binding.binding.vulkan == 1 || binding.binding.vulkan == 2 || binding.binding.vulkan == 3 ||
            binding.binding.vulkan == 9;
        const bool array = binding.binding.vulkan == 16 || binding.binding.vulkan == 24;
        require(binding.set == (array ? 2u : stable ? 1u : 0u), "Shadow v5 layout split differs");
    }
    auto drawable = std::make_shared<Drawable>();
    drawable->setMesh(geo::createPlane(2,2));
    drawable->material().setAlphaCutoff(0.5f);
    drawable->load(&engine);
    auto node = std::make_shared<scene::Node>("shadow bindings probe");
    RenderQueue queue;
    const auto extract = [&]()
    {
        queue.beginFrame(engine);
        queue.enqueue(*node, drawable, 0, glm::mat4(1));
    };
    extract();
    TracingSceneResources resources(device, capacity);
    const auto snapshot = resources.prepare(queue, capacity);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        snapshot->build(*command);
    });
    const auto layout = device.createPipelineLayout(description);
    bool rejected = false;
    try
    {
        snapshot->shadowAlphaResourceSet(layout, capacity-1);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "Array count mismatch must fail without publishing a reusable set");
    auto first = snapshot->shadowAlphaResourceSet(layout, capacity);
    require(first == snapshot->shadowAlphaResourceSet(layout, capacity), "Stable arrays must be reused within one snapshot/layout");
    const auto fresh = snapshot->shadowAlphaResourceSet(layout, capacity, false);
    require(fresh != first && first == snapshot->shadowAlphaResourceSet(layout, capacity), "Diagnostic fallback must not replace shared sets");
    const auto otherLayout = device.createPipelineLayout(description);
    require(snapshot->shadowAlphaResourceSet(otherLayout, capacity) != first, "Layout identity must distinguish shadow bindings");
    const auto smallerLayout = device.createPipelineLayout(softShadowBindingLayout(capacity-1));
    require(snapshot->shadowAlphaResourceSet(smallerLayout, capacity-1) != first, "Distinct valid capacities must have distinct stable sets");
#if BG2E_IS_MAC
    if (auto* native = dynamic_cast<gpu::metal::ResourceSet*>(fresh.get()))
    {
        auto* nativeLayout = dynamic_cast<gpu::metal::PipelineLayout*>(layout.get());
        require(nativeLayout && nativeLayout->argumentEncoder(5) == nativeLayout->argumentEncoder(5), "Compatible encoders must be cached per layout group");
        const auto bufferAt = [&](const gpu::metal::ResourceSet& set, uint32_t index)
        {
            for (const auto& entry : set.entries())
            {
                if (entry.index == index && entry.type == gpu::ResourceType::StorageBuffer)
                {
                    return entry.buffer;
                }
            }
            return gpu::metal::BufferHandle(nullptr);
        };
        const auto imageBuffer = bufferAt(*native, 5);
        const auto samplerBuffer = bufferAt(*native, 6);
        const auto* cachedNative = dynamic_cast<gpu::metal::ResourceSet*>(first.get());
        require(cachedNative && imageBuffer && samplerBuffer && imageBuffer != bufferAt(*cachedNative,5),
            "Shared encoder state must not share mutable argument-buffer storage between sets");
        std::vector<gpu::ImageView> views(capacity, snapshot->shadowFallbackImage->fullView());
        std::vector<gpu::Sampler*> samplers(capacity, snapshot->shadowFallbackSampler.get());
        for (size_t i = 0; i < snapshot->shadowImages.size(); ++i)
        {
            views[i] = snapshot->shadowImages[i]->fullView();
            samplers[i] = snapshot->shadowSamplers[i].get();
        }
        native->setSampledImages({16,0}, views);
        native->setSamplers({24,0}, samplers);
        native->update();
        require(imageBuffer == bufferAt(*native,5) && samplerBuffer == bufferAt(*native,6),
            "Unchanged arrays must preserve native argument-buffer storage");
        bool resident = false;
        for (const auto& entry : native->entries())
        {
            resident |= entry.index == 5 && !entry.indirectTextures.empty();
        }
        require(resident, "Reused argument buffers must retain indirect texture residency");
    }
#endif
    // Exceed the per-tuple layout cache bound. Retained sets stay valid after
    // eviction, while an evicted key must allocate a new immutable native set.
    for (uint32_t i = 0; i < 9; ++i)
    {
        const auto temporaryLayout = device.createPipelineLayout(description);
        snapshot->shadowAlphaResourceSet(temporaryLayout, capacity);
    }
    const auto afterEviction = snapshot->shadowAlphaResourceSet(layout, capacity);
    require(afterEviction != first && first->isValid(), "Bounded cache eviction must preserve retained sets");
    first = afterEviction;
    // Reload the mutable source wrapper. The old snapshot must keep its original
    // native resources instead of resolving the wrapper again on a cache hit.
    const auto oldImage = snapshot->shadowFallbackImage;
    drawable->drawMaterial(0)->albedoTexture()->reload();
    require(oldImage != drawable->drawMaterial(0)->albedoTexture()->imageResource() && oldImage->isValid() &&
        first == snapshot->shadowAlphaResourceSet(layout, capacity), "Retained shadow owners must survive source texture reload");
    drawable->material().setAlphaCutoff(0);
    extract();
    const auto opaque = resources.prepare(queue, capacity);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        opaque->build(*command);
    });
    require(opaque->shadowImages.empty() && opaque->shadowFallbackImage && opaque->shadowAlphaResourceSet(layout, capacity),
        "All-opaque tables must still fully initialize device-sized fallback arrays");
    oldImage->cleanup(); // Completed, probe-owned resource; exercise stale generation/validity rejection.
    rejected = false;
    try
    {
        snapshot->shadowAlphaResourceSet(layout, capacity);
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected, "Native recreation/cleanup must not silently hit a cached shadow set");
    return true;
}
}
