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

#include <bg2e/gpu/vk/ResourceSet.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/Sampler.hpp>
#include <bg2e/gpu/vk/Buffer.hpp>
#include <bg2e/gpu/vk/RayTracingScene.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

ResourceSet::ResourceSet(gpu::Device* gpuDevice, VkDevice device, const vk::PipelineLayout* layout, uint32_t setIndex, const std::string& debugName)
    : gpu::ResourceSet(gpuDevice)
    , _device(device)
    , _setIndex(setIndex)
{
    VkDescriptorSetLayout dsLayout = layout->descriptorSetLayout(setIndex);
    if (dsLayout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("vk::ResourceSet: invalid set index, no descriptor set layout");
    }

    // Create a descriptor pool sized for this set's bindings
    const auto& desc = layout->description();
    uint32_t sampledImageCount = 0;
    uint32_t storageImageCount = 0;
    uint32_t samplerCount = 0;
    uint32_t uniformBufferCount = 0;
    uint32_t storageBufferCount = 0;
    uint32_t accelerationStructureCount = 0;

    for (const auto& rb : desc.resourceBindings)
    {
        if (rb.set != setIndex) continue;
        switch (rb.type)
        {
            case ResourceType::SampledImage:  sampledImageCount += rb.count; break;
            case ResourceType::StorageImage:  storageImageCount += rb.count; break;
            case ResourceType::Sampler:       samplerCount += rb.count; break;
            case ResourceType::UniformBuffer: uniformBufferCount += rb.count; break;
            case ResourceType::StorageBuffer: storageBufferCount += rb.count; break;
            case ResourceType::AccelerationStructure: accelerationStructureCount += rb.count; break;
            default: break;
        }
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    if (sampledImageCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledImageCount });
    if (storageImageCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageCount });
    if (samplerCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, samplerCount });
    if (uniformBufferCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount });
    if (storageBufferCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount });
    if (accelerationStructureCount > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, accelerationStructureCount });

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool);
    if (result != VK_SUCCESS)
    {
        bg2e_log_error << "vk::ResourceSet: vkCreateDescriptorPool failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan descriptor pool");
    }

    if (base::Log::isDebug() && !debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_descriptorPool);
        nameInfo.pObjectName = debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    result = vkAllocateDescriptorSets(_device, &allocInfo, &_descriptorSet);
    if (result != VK_SUCCESS)
    {
        bg2e_log_error << "vk::ResourceSet: vkAllocateDescriptorSets failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to allocate Vulkan descriptor set");
    }

    if (base::Log::isDebug() && !debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_descriptorSet);
        nameInfo.pObjectName = debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
    }
}

ResourceSet::~ResourceSet()
{
    cleanup();
}

void ResourceSet::setStorageImage(ShaderBinding binding, gpu::Image* image)
{
    auto* vkImage = dynamic_cast<vk::Image*>(image);
    if (!vkImage)
    {
        throw std::runtime_error("vk::ResourceSet::setStorageImage: invalid image");
    }

    const size_t infoIndex = _imageInfos.size();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = vkImage->imageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    _imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    // pImageInfo is resolved in update() to avoid dangling pointers.
    write.pImageInfo = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Image);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::setSampledImage(ShaderBinding binding, gpu::Image* image)
{
    auto* vkImage = dynamic_cast<vk::Image*>(image);
    if (!vkImage)
    {
        throw std::runtime_error("vk::ResourceSet::setSampledImage: invalid image");
    }

    const size_t infoIndex = _imageInfos.size();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = vkImage->imageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    _imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.descriptorCount = 1;
    // pImageInfo is resolved in update() to avoid dangling pointers.
    write.pImageInfo = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Image);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::setSampler(ShaderBinding binding, gpu::Sampler* sampler)
{
    auto* vkSampler = dynamic_cast<vk::Sampler*>(sampler);
    if (!vkSampler)
    {
        throw std::runtime_error("vk::ResourceSet::setSampler: invalid sampler");
    }

    const size_t infoIndex = _imageInfos.size();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = vkSampler->handle();

    _imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.descriptorCount = 1;
    // pImageInfo is resolved in update() to avoid dangling pointers.
    write.pImageInfo = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Image);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::setUniformBuffer(ShaderBinding binding, gpu::Buffer* buffer)
{
    auto* vkBuffer = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuffer)
    {
        throw std::runtime_error("vk::ResourceSet::setUniformBuffer: invalid buffer");
    }

    const size_t infoIndex = _bufferInfos.size();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->handle();
    bufferInfo.offset = 0;
    bufferInfo.range  = vkBuffer->byteSize();

    _bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    // pBufferInfo is resolved in update() to avoid dangling pointers.
    write.pBufferInfo = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Buffer);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::setStorageBuffer(ShaderBinding binding, gpu::Buffer* buffer)
{
    auto* vkBuffer = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuffer)
    {
        throw std::runtime_error("vk::ResourceSet::setStorageBuffer: invalid buffer");
    }

    const size_t infoIndex = _bufferInfos.size();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->handle();
    bufferInfo.offset = 0;
    bufferInfo.range  = vkBuffer->byteSize();

    _bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    // pBufferInfo is resolved in update() to avoid dangling pointers.
    write.pBufferInfo = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Buffer);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::setRayTracingScene(ShaderBinding binding, gpu::RayTracingScene* scene)
{
    auto* vkScene = dynamic_cast<vk::RayTracingScene*>(scene);
    if (!vkScene)
    {
        throw std::runtime_error("vk::ResourceSet::setRayTracingScene: invalid ray tracing scene");
    }

    const size_t infoIndex = _asHandles.size();
    _asHandles.push_back(vkScene->handle());

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.descriptorCount = 1;
    // pNext (acceleration structure write) is resolved in update() to avoid
    // dangling pointers caused by vector reallocation.
    write.pNext = nullptr;

    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::AccelerationStructure);
    _writeInfoIndices.push_back(infoIndex);
}

void ResourceSet::update()
{
    if (_pendingWrites.empty())
    {
        return;
    }

    // Now that the info vectors have stopped growing their addresses are stable,
    // so bind each write to its descriptor info. Resolving the pointers here
    // (instead of in the set* methods) avoids dangling pointers caused by vector
    // reallocation while bindings were being added.
    //
    // One VkWriteDescriptorSetAccelerationStructureKHR is needed per
    // acceleration structure write; allocate them up front so their addresses
    // remain stable for the vkUpdateDescriptorSets call below.
    _asInfos.assign(_asHandles.size(), VkWriteDescriptorSetAccelerationStructureKHR{});

    for (size_t i = 0; i < _pendingWrites.size(); ++i)
    {
        switch (_writeInfoKinds[i])
        {
            case WriteInfoKind::Image:
                _pendingWrites[i].pImageInfo = &_imageInfos[_writeInfoIndices[i]];
                break;
            case WriteInfoKind::Buffer:
                _pendingWrites[i].pBufferInfo = &_bufferInfos[_writeInfoIndices[i]];
                break;
            case WriteInfoKind::AccelerationStructure:
            {
                const size_t idx = _writeInfoIndices[i];
                VkWriteDescriptorSetAccelerationStructureKHR& asInfo = _asInfos[idx];
                asInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                asInfo.accelerationStructureCount = 1;
                asInfo.pAccelerationStructures    = &_asHandles[idx];
                _pendingWrites[i].pNext = &asInfo;
                break;
            }
        }
    }

    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(_pendingWrites.size()),
                           _pendingWrites.data(), 0, nullptr);

    _pendingWrites.clear();
    _imageInfos.clear();
    _bufferInfos.clear();
    _asHandles.clear();
    _asInfos.clear();
    _writeInfoKinds.clear();
    _writeInfoIndices.clear();
}

bool ResourceSet::isValid() const
{
    return _descriptorSet != VK_NULL_HANDLE;
}

void ResourceSet::cleanup()
{
    _pendingWrites.clear();
    _imageInfos.clear();
    _bufferInfos.clear();
    _asHandles.clear();
    _asInfos.clear();
    _writeInfoKinds.clear();
    _writeInfoIndices.clear();

    if (_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
        _descriptorPool = VK_NULL_HANDLE;
        _descriptorSet = VK_NULL_HANDLE;
    }
}

}
}
}
