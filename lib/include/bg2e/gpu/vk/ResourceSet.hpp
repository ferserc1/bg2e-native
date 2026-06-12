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

#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <vector>

namespace bg2e {
namespace gpu {
namespace vk {

class PipelineLayout;

class BG2E_API ResourceSet : public gpu::ResourceSet {
public:
    ResourceSet(VkDevice device, const vk::PipelineLayout* layout, uint32_t setIndex);
    ~ResourceSet() override;

    void setStorageImage(uint32_t binding, gpu::Image* image) override;
    void setSampledImage(uint32_t binding, gpu::Image* image) override;
    void setSampler(uint32_t binding, gpu::Sampler* sampler) override;

    void update() override;

    uint32_t setIndex() const override { return _setIndex; }
    bool isValid() const override;
    void cleanup() override;

    VkDescriptorSet handle() const { return _descriptorSet; }

private:
    VkDevice        _device{VK_NULL_HANDLE};
    uint32_t        _setIndex{0};
    VkDescriptorPool _descriptorPool{VK_NULL_HANDLE};
    VkDescriptorSet  _descriptorSet{VK_NULL_HANDLE};

    // Intermediate storage kept alive until update() is called. pImageInfo is
    // not bound in the set* methods because pushing into _imageInfos may
    // reallocate and invalidate pointers to earlier elements; instead each
    // write stores the index of its descriptor info and the pointers are
    // resolved in update(), once the vector has stopped growing.
    std::vector<VkDescriptorImageInfo> _imageInfos;
    std::vector<VkWriteDescriptorSet>  _pendingWrites;
    std::vector<size_t>                _writeImageInfoIndices;
};

}
}
}
