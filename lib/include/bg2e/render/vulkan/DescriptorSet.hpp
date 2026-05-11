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

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/Texture.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class Buffer;

class BG2E_API DescriptorSet {
public:

    void init(Engine * e, VkDescriptorSet ds);

    // To update a descriptor set:
    // - call beginUpdate()
    // - call addImage, addBuffer and/or addAccelerationStructure until you
    //   complete the descriptor set info
    // - call endUpdate()

    // OR

    // If the descriptor set only have one image
    // or one buffer, you can use the funtions
    // updateImage() and updateBuffer(), that
    // does the previous steps for you in one call

    inline void updateImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageView imageView,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    ) {
        beginUpdate();
        addImage(
            binding,
            type,
            imageView,
            layout,
            sampler
        );
        endUpdate();
    }

    void updateBuffer(
        uint32_t binding,
        VkDescriptorType type,
        Buffer* buffer,
        size_t size,
        size_t offset
    );

    inline void beginUpdate() { clear(); }

    inline void addImage(
        uint32_t binding,
        VkDescriptorType type,
        const Image* image,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    ) {
        addImage(binding, type, image->imageView(), layout, sampler);
    }

    void addImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageView imageView,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    );

    void addImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageLayout layout,
        render::Texture* texture
    ) {
        addImage(binding, type, texture->image()->imageView(), layout, texture->sampler());
    }

    void addImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageLayout layout,
        const std::shared_ptr<render::Texture>& texture
    ) {
        addImage(binding, type, texture->image()->imageView(), layout, texture->sampler());
    }

    void addBuffer(
        uint32_t binding,
        VkDescriptorType type,
        Buffer* buffer,
        size_t size,
        size_t offset
    );

    void addBuffer(
        uint32_t binding,
        VkDescriptorType type,
        VkBuffer buffer,
        size_t size,
        size_t offset
    );

    void addAccelerationStructure(
        uint32_t binding,
        VkAccelerationStructureKHR accelerationStructure
    );

    void endUpdate();

    // Clear all descriptor writes to add images and buffers again
    void clear();

    inline VkDescriptorSet descriptorSet() const { return _ds; }

    inline VkDescriptorSet* operator&() { return &_ds; }

protected:
    Engine * _engine = nullptr;

    std::deque<VkDescriptorImageInfo> _imageInfos;
    std::deque<VkDescriptorBufferInfo> _bufferInfos;
    std::deque<VkAccelerationStructureKHR> _accelerationStructures;
    std::deque<VkWriteDescriptorSetAccelerationStructureKHR> _asInfo;
    std::vector<VkWriteDescriptorSet> _writes;

    //VkDescriptorImageInfo _imageInfo;
    //VkDescriptorBufferInfo _bufferInfo;
    VkDescriptorSet _ds = VK_NULL_HANDLE;
};
}
}
}
