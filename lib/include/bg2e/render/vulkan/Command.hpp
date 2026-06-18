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

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Device.hpp>
#include <functional>


namespace bg2e {
namespace render {

class Engine;

namespace vulkan {

class BG2E_API Command {
public:
    void init(Engine * engine);
    
    VkCommandPool createCommandPool(VkCommandPoolCreateFlags flags);
    
    VkCommandBuffer allocateCommandBuffer(VkCommandPool pool, uint32_t count = 1);
    
    void destroyComandPool(VkCommandPool pool);
    
    VkDevice device() const;
    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

protected:
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _immediateQueue = VK_NULL_HANDLE;
    uint32_t _graphicsQueueFamily = 0;

    VkCommandPool _immediateCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer _immediateCmdBuffer = VK_NULL_HANDLE;
    VkFence _immediateCmdFence = VK_NULL_HANDLE;


    Engine * _engine;
};

}
}
}
