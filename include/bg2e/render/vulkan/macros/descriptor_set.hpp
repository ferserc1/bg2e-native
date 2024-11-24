#pragma once

#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/Vulkan.hpp>

#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

template <typename T>
DescriptorSet* uniformBufferDescriptorSet(Vulkan* vulkan, FrameResources& frameResources, VkDescriptorSetLayout descriptorSetLayout, uint32_t binding, const T& data, uint32_t currentFrame)
{
     auto uniformBuffer = Buffer::createAllocatedBuffer(
        vulkan,
        sizeof(T),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );
    
    auto dataPtr = reinterpret_cast<T*>(uniformBuffer->allocatedData());
    *dataPtr = data;
    
    auto descriptorSet = frameResources.descriptorAllocator->allocate(descriptorSetLayout);
    descriptorSet->updateBuffer(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer, sizeof(T), 0);
    frameResources.cleanupManager.push([&, uniformBuffer](VkDevice dev) {
        uniformBuffer->cleanup();
        delete uniformBuffer;
    });
    return descriptorSet;
}

}
}
}
}

