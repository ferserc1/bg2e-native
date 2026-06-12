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

#include <bg2e/gpu/Queue.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <memory>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;

class Queue : public gpu::Queue {
public:
    Queue() = default;
    Queue(VkQueue queue, uint32_t family);

    uint32_t familyIndex() const override;
    bool isValid() const override;

    VkQueue handle() const;

    void initCommandPool(VkDevice device, vk::Device* gpuDevice);
    void destroyCommandPool();

    std::shared_ptr<gpu::CommandBuffer> createCommandBuffer(const std::string& debugName = {}) const override;
    void submit(gpu::CommandBuffer* cmd) const override;

private:
    VkQueue       _queue{VK_NULL_HANDLE};
    uint32_t      _familyIndex{UINT32_MAX};
    VkDevice      _device{VK_NULL_HANDLE};
    vk::Device*   _gpuDevice{nullptr};
    VkCommandPool _commandPool{VK_NULL_HANDLE};
};

}
}
}
