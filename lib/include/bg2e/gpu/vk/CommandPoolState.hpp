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

#include <bg2e/gpu/vk/common.hpp>
#include <memory>
#include <mutex>

namespace bg2e::gpu::vk {

// Shared allocation lifetime; explicit cleanup invalidates surviving wrappers.
struct CommandPoolState {
    std::mutex mutex;
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;

    ~CommandPoolState() { cleanup(); }

    void cleanup() {
        std::lock_guard lock(mutex);
        if (pool) vkDestroyCommandPool(device, pool, nullptr);
        pool = VK_NULL_HANDLE;
        device = VK_NULL_HANDLE;
    }
};

struct CommandAllocation {
    std::shared_ptr<CommandPoolState> pool;
    VkCommandBuffer command = VK_NULL_HANDLE;
    ~CommandAllocation() {
        if (!command) return;
        std::lock_guard lock(pool->mutex);
        if (pool->pool && command)
            vkFreeCommandBuffers(pool->device, pool->pool, 1, &command);
    }
};

}
