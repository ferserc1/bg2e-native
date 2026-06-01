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

#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>

#include <vulkan/vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

void DescriptorSet::init(
    Engine * engine,
    VkDescriptorSet ds
) {
    _engine = engine;
    _ds = ds;
}

void DescriptorSet::updateBuffer(
    uint32_t binding,
    VkDescriptorType type,
    Buffer* buffer,
    size_t size,
    size_t offset
) {
    beginUpdate();
    addBuffer(
        binding,
        type,
        buffer,
        size,
        offset
    );
    endUpdate();
}

void DescriptorSet::addImage(
    uint32_t binding,
    VkDescriptorType type,
    VkImageView imageView,
    VkImageLayout layout,
    VkSampler sampler
) {
    
    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageView = imageView;
    imgInfo.imageLayout = layout;
    imgInfo.sampler = sampler;
    VkDescriptorImageInfo& info = _imageInfos.emplace_back(imgInfo);
    
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;
    
    _writes.push_back(write);
}


void DescriptorSet::addBuffer(
    uint32_t binding,
    VkDescriptorType type,
    VkBuffer buffer,
    size_t size,
    size_t offset
) {
    VkDescriptorBufferInfo bufInfo = {};
    bufInfo.buffer = buffer;
    bufInfo.offset = offset;
    bufInfo.range = size;
    VkDescriptorBufferInfo& info = _bufferInfos.emplace_back(bufInfo);
    
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;
    _writes.push_back(write);
}

void DescriptorSet::addBuffer(
    uint32_t binding,
    VkDescriptorType type,
    Buffer* buffer,
    size_t size,
    size_t offset
) {
    addBuffer(binding, type, buffer->handle(), size, offset);
}

void DescriptorSet::addAccelerationStructure(
    uint32_t binding,
    VkAccelerationStructureKHR accelerationStructure
) {
    _accelerationStructures.push_back(accelerationStructure);

    _asInfo.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &_accelerationStructures.back()
    });

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pNext = &_asInfo.back();

    _writes.push_back(write);
}

void DescriptorSet::addBufferArray(
    uint32_t binding,
    VkDescriptorType type,
    const std::vector<VkBuffer>& buffers,
    const std::vector<size_t>& sizes,
    const std::vector<size_t>& offsets
) {
    auto& infos = _bufferInfoArrays.emplace_back(buffers.size());
    for (size_t i = 0; i < buffers.size(); ++i)
    {
        infos[i].buffer = buffers[i];
        infos[i].offset = offsets[i];
        infos[i].range = sizes[i];
    }

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = static_cast<uint32_t>(buffers.size());
    write.descriptorType = type;
    write.pBufferInfo = infos.data();

    _writes.push_back(write);
}

void DescriptorSet::addImageArray(
    uint32_t binding,
    VkDescriptorType type,
    const std::vector<VkImageView>& imageViews,
    const std::vector<VkSampler>& samplers,
    VkImageLayout layout
) {
    auto& infos = _imageInfoArrays.emplace_back(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        infos[i].sampler = samplers[i];
        infos[i].imageView = imageViews[i];
        infos[i].imageLayout = layout;
    }

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = static_cast<uint32_t>(imageViews.size());
    write.descriptorType = type;
    write.pImageInfo = infos.data();

    _writes.push_back(write);
}

void DescriptorSet::endUpdate()
{
    vkUpdateDescriptorSets(_engine->device().handle(), uint32_t(_writes.size()), _writes.data(), 0, nullptr);
}

void DescriptorSet::clear()
{
    _imageInfos.clear();
    _bufferInfos.clear();
    _accelerationStructures.clear();
    _asInfo.clear();
    _bufferInfoArrays.clear();
    _imageInfoArrays.clear();
    _writes.clear();
}

}
}
}

