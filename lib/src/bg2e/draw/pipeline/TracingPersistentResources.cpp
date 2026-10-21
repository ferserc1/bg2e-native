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

#include "TracingPersistentResources.hpp"
#include <bg2e/base/CpuProfile.hpp>

namespace bg2e::draw::pipeline {
bool TracingResourceTable::matches(const TracingResourceTable& other) const
{
    return device == other.device && admissionCapacity == other.admissionCapacity && images == other.images &&
        samplers == other.samplers && generations == other.generations && fallbackImage == other.fallbackImage &&
        fallbackSampler == other.fallbackSampler && fallbackGeneration == other.fallbackGeneration;
}

void TracingResourceTable::validate() const
{
    if (!device || images.size() != generations.size() || !fallbackImage || !fallbackSampler ||
        !fallbackImage->isValid() || !fallbackSampler->isValid() || fallbackImage->device() != device ||
        fallbackSampler->device() != device || fallbackImage->bindingGeneration() != fallbackGeneration)
    {
        throw std::logic_error("Discard tracing resources after native resource recreation");
    }
    for (size_t i = 0; i < images.size(); ++i)
    {
        if (!images[i] || !images[i]->isValid() || images[i]->device() != device ||
            images[i]->bindingGeneration() != generations[i])
        {
            throw std::logic_error("Discard tracing resources after native image recreation");
        }
    }
    for (const auto& sampler : samplers)
    {
        if (!sampler || !sampler->isValid() || sampler->device() != device)
        {
            throw std::logic_error("Discard tracing resources after sampler cleanup");
        }
    }
}

std::shared_ptr<gpu::ResourceSet> TracingResourceTable::bindings(gpu::PipelineLayout& layout,
    uint32_t capacity, bool shadow, bool reuse) const
{
    validate();
    if (!layout.isValid() || layout.device() != device || !capacity || capacity > gpu::MaxSampledResourceTableCapacity ||
        images.size() > capacity || samplers.size() > capacity)
    {
        throw std::invalid_argument("Persistent arrays require a compatible layout and full capacity");
    }
    // Retained handles keep the tuple and layout alive independently of cache eviction.
    // Cache entries deliberately do not own this holder, avoiding a reference cycle.
    struct Retained {
        std::shared_ptr<const TracingResourceTable> table;
        std::shared_ptr<gpu::DeviceResource> layout;
        std::shared_ptr<gpu::ResourceSet> set;
    };
    const auto layoutOwner = layout.weak_from_this().lock();
    const auto retain = [&](const std::shared_ptr<gpu::ResourceSet>& set)
    {
        auto owner = std::make_shared<Retained>(Retained{shared_from_this(), layoutOwner, set});
        return std::shared_ptr<gpu::ResourceSet>(owner, set.get());
    };
    std::lock_guard lock(_mutex);
    // Borrowed layouts cannot safely participate in pointer-identity caching.
    if (reuse && layoutOwner)
    {
        for (const auto& entry : _bindings)
        {
            if (entry.layout == &layout && entry.capacity == capacity && entry.shadow == shadow)
            {
                base::CpuProfileScope::count("tracing.persistentArrayBindingHits", 1);
                return retain(entry.set);
            }
        }
    }
    auto set = device->createResourceSet(&layout, 2);
    std::vector<gpu::ImageView> views(capacity, fallbackImage->fullView());
    std::vector<gpu::Sampler*> states(capacity, fallbackSampler.get());
    for (size_t i = 0; i < images.size(); ++i)
    {
        views[i] = images[i]->fullView();
    }
    for (size_t i = 0; i < samplers.size(); ++i)
    {
        states[i] = samplers[i].get();
    }
    set->setSampledImages({shadow ? 16u : 5u, 0}, views);
    set->setSamplers({shadow ? 24u : 6u, 0}, states);
    set->update();
    auto retained = retain(set);
    if (reuse && layoutOwner)
    {
        auto updated = _bindings;
        constexpr size_t maxBindings = 8;
        if (updated.size() == maxBindings)
        {
            updated.erase(updated.begin());
        }
        updated.push_back({&layout, layoutOwner, capacity, shadow, set});
        _bindings.swap(updated);
    }
    base::CpuProfileScope::count("tracing.persistentArrayPreparations", 1);
    if (shadow)
    {
        base::CpuProfileScope::count("shadow.alphaArrayPreparations", 1);
        base::CpuProfileScope::count("shadow.alphaArrayOccupiedSlots", images.size());
        base::CpuProfileScope::count("shadow.alphaArrayCapacitySlots", capacity);
    }
    return retained;
}

bool TracingAccelerationGeneration::completed() const
{
    if (!recorded || !completion || !completion->completed())
    {
        return false;
    }
    if (!scene || !scene->isValid() || !buffer || !buffer->isValid() || !geometry ||
        !geometry->vertexBuffer->isValid() || !geometry->indexBuffer->isValid())
    {
        throw std::logic_error("Discard acceleration generations after native cleanup");
    }
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (!blasOwners[i]->isValid() || !blasOwners[i]->isBuilt() || blasOwners[i]->buildGeneration() != keys[i].blasGeneration)
        {
            return false;
        }
    }
    return true;
}

}
