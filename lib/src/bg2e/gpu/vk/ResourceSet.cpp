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
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/Sampler.hpp>
#include <bg2e/gpu/vk/Buffer.hpp>
#include <bg2e/gpu/vk/RayTracingScene.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/gpu/Device.hpp>
#include <algorithm>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

ResourceSet::ResourceSet(gpu::Device* gpuDevice, VkDevice device, const vk::PipelineLayout* layout, uint32_t setIndex, const std::string& debugName)
    : gpu::ResourceSet(gpuDevice), _device(device), _setIndex(setIndex)
{
    base::CpuProfileScope profile("vulkan.resourceSet.allocatePoolAndSet");
    _bindings = layout->description().resourceBindings;
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
        if (rb.set != setIndex)
        {
            continue;
        }
        switch (rb.type)
        {
        case ResourceType::SampledImage:
            sampledImageCount += rb.count;
            break;
        case ResourceType::StorageImage:
            storageImageCount += rb.count;
            break;
        case ResourceType::Sampler:
            samplerCount += rb.count;
            break;
        case ResourceType::UniformBuffer:
            uniformBufferCount += rb.count;
            break;
        case ResourceType::StorageBuffer:
            storageBufferCount += rb.count;
            break;
        case ResourceType::AccelerationStructure:
            accelerationStructureCount += rb.count;
            break;
        default:
            break;
        }
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    if (sampledImageCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledImageCount});
    }
    if (storageImageCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageCount});
    }
    if (samplerCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, samplerCount});
    }
    if (uniformBufferCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount});
    }
    if (storageBufferCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount});
    }
    if (accelerationStructureCount > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, accelerationStructureCount});
    }

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
        cleanup();
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
    if (!image)
    {
        throw std::invalid_argument("Image binding requires an image");
    }
    setStorageImage(binding, image->view({0, 1, 0, image->arrayLayers()}));
}

void ResourceSet::setStorageImage(ShaderBinding binding, const ImageView& view)
{
    bindingDescription(binding, ResourceType::StorageImage);
    auto* image = view.image;
    if (!image || image->device() != device() || !hasFlag(image->usage(), ImageUsage::Storage))
    {
        throw std::invalid_argument("Image view device/usage is incompatible with this resource set");
    }
    image->view(view.range);
    if (view.range.mipCount != 1 || isDepthFormat(image->pixelFormat()))
    {
        throw std::invalid_argument("A storage image view selects exactly one color mip");
    }
    auto* vkImage = dynamic_cast<vk::Image*>(image);
    if (!vkImage)
    {
        throw std::runtime_error("vk::ResourceSet::setStorageImage: invalid image");
    }

    const size_t infoIndex = _imageInfos.size();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = vkImage->subresourceView(view.range);
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
    if (!image)
    {
        throw std::invalid_argument("Image binding requires an image");
    }
    setSampledImage(binding, image->fullView());
}

ResourceBinding ResourceSet::bindingDescription(ShaderBinding binding, ResourceType type, size_t count) const
{
    for (const auto& rb : _bindings)
    {
        if (rb.set == _setIndex && rb.binding.vulkan == binding.vulkan && rb.type == type && rb.count == count)
        {
            return rb;
        }
    }
    throw std::invalid_argument("Resource binding type/count does not match its layout");
}

void ResourceSet::setSampledImage(ShaderBinding binding, const ImageView& view)
{
    setSampledImages(binding, std::span<const ImageView>(&view, 1));
}

void ResourceSet::setSampler(ShaderBinding binding, gpu::Sampler* sampler)
{
    setSamplers(binding, std::span<gpu::Sampler* const>(&sampler, 1));
}

void ResourceSet::setSampledImages(ShaderBinding binding, std::span<const ImageView> views)
{
    base::CpuProfileScope profile("vulkan.resourceSet.resolveImageViews");
    bindingDescription(binding, ResourceType::SampledImage, views.size());
    std::vector<VkDescriptorImageInfo> infos;
    for (const auto& view : views)
    {
        auto* image = dynamic_cast<vk::Image*>(view.image);
        if (!image || image->device() != device() || !image->isValid() || !hasFlag(image->usage(), ImageUsage::Sampled) ||
            (views.size() > 1 && (image->imageType() != ImageType::Image2D || isDepthFormat(image->pixelFormat()))))
        {
            throw std::invalid_argument("Sampled image device/type/usage is incompatible");
        }
        image->validateRange(view.range);
        infos.push_back({VK_NULL_HANDLE, image->subresourceView(view.range), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
    }
    const size_t start = _imageInfos.size();
    _imageInfos.insert(_imageInfos.end(), infos.begin(), infos.end());
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.descriptorCount = static_cast<uint32_t>(views.size());
    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Image);
    _writeInfoIndices.push_back(start);
    if (views.size() > 1)
    {
        _pendingArrays.push_back(binding.vulkan);
    }
}

void ResourceSet::setSamplers(ShaderBinding binding, std::span<gpu::Sampler* const> samplers)
{
    bindingDescription(binding, ResourceType::Sampler, samplers.size());
    std::vector<VkDescriptorImageInfo> infos;
    for (auto* value : samplers)
    {
        auto* sampler = dynamic_cast<vk::Sampler*>(value);
        if (!sampler || sampler->device() != device() || !sampler->isValid())
        {
            throw std::invalid_argument("Sampler device/state is incompatible");
        }
        infos.push_back({sampler->handle(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED});
    }
    const size_t start = _imageInfos.size();
    _imageInfos.insert(_imageInfos.end(), infos.begin(), infos.end());
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = binding.vulkan;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.descriptorCount = static_cast<uint32_t>(samplers.size());
    _pendingWrites.push_back(write);
    _writeInfoKinds.push_back(WriteInfoKind::Image);
    _writeInfoIndices.push_back(start);
    if (samplers.size() > 1)
    {
        _pendingArrays.push_back(binding.vulkan);
    }
}

void ResourceSet::setUniformBuffer(ShaderBinding binding, gpu::Buffer* buffer)
{
    bindingDescription(binding, ResourceType::UniformBuffer);
    auto* vkBuffer = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuffer || vkBuffer->device() != device() || !vkBuffer->isValid())
    {
        throw std::runtime_error("vk::ResourceSet::setUniformBuffer: invalid buffer");
    }

    if (!hasFlag(buffer->usage(), BufferUsage::Uniform) || !buffer->byteSize() ||
        (hasFlag(buffer->usage(), BufferUsage::Storage) && buffer->byteSize() > device()->resourceArrayCapabilities().maxStorageBufferRange))
    {
        throw std::invalid_argument("Buffer range/usage is incompatible");
    }
    const size_t infoIndex = _bufferInfos.size();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->handle();
    bufferInfo.offset = 0;
    bufferInfo.range = vkBuffer->byteSize();

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
    bindingDescription(binding, ResourceType::StorageBuffer);
    auto* vkBuffer = dynamic_cast<vk::Buffer*>(buffer);
    if (!vkBuffer || vkBuffer->device() != device() || !vkBuffer->isValid())
    {
        throw std::runtime_error("vk::ResourceSet::setStorageBuffer: invalid buffer");
    }

    if (!hasFlag(buffer->usage(), BufferUsage::Storage) || !buffer->byteSize() ||
        (hasFlag(buffer->usage(), BufferUsage::Storage) && buffer->byteSize() > device()->resourceArrayCapabilities().maxStorageBufferRange))
    {
        throw std::invalid_argument("Buffer range/usage is incompatible");
    }
    const size_t infoIndex = _bufferInfos.size();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->handle();
    bufferInfo.offset = 0;
    bufferInfo.range = vkBuffer->byteSize();

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
    bindingDescription(binding, ResourceType::AccelerationStructure);
    auto* vkScene = dynamic_cast<vk::RayTracingScene*>(scene);
    if (!vkScene || vkScene->device() != device() || !vkScene->isValid())
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
    base::CpuProfileScope profile("vulkan.resourceSet.updateDescriptors");
    for (const auto& rb : _bindings)
    {
        if (rb.set == _setIndex && rb.count > 1 &&
            std::find(_initializedArrays.begin(), _initializedArrays.end(), rb.binding.vulkan) == _initializedArrays.end() &&
            std::find(_pendingArrays.begin(), _pendingArrays.end(), rb.binding.vulkan) == _pendingArrays.end())
        {
            throw std::invalid_argument("Every array element must be initialized before update");
        }
    }
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
        case WriteInfoKind::AccelerationStructure: {
            const size_t idx = _writeInfoIndices[i];
            VkWriteDescriptorSetAccelerationStructureKHR& asInfo = _asInfos[idx];
            asInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            asInfo.accelerationStructureCount = 1;
            asInfo.pAccelerationStructures = &_asHandles[idx];
            _pendingWrites[i].pNext = &asInfo;
            break;
        }
        }
    }

    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(_pendingWrites.size()),
                           _pendingWrites.data(), 0, nullptr);

    for (auto binding : _pendingArrays)
    {
        if (std::find(_initializedArrays.begin(), _initializedArrays.end(), binding) == _initializedArrays.end())
        {
            _initializedArrays.push_back(binding);
        }
    }
    _pendingArrays.clear();
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
    base::CpuProfileScope profile("vulkan.resourceSet.cleanup");
    _initializedArrays.clear();
    _pendingArrays.clear();
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
