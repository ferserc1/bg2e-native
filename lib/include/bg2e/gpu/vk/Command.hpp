/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/Command.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Command : public gpu::Command {
public:
    void init(gpu::Device* device) override;

    void immediateSubmit(std::function<void (gpu::Command *)>) override;

    inline VkCommandPool vkCmdPool() const { return _immCmdPool; }
    inline VkCommandBuffer vkCmdBuffer() const { return _immCmdBuffer; }
    inline VkFence vkCmdFence() const { return _immCmdFence; }

private:
    VkCommandPool _immCmdPool{VK_NULL_HANDLE};
    VkCommandBuffer _immCmdBuffer{VK_NULL_HANDLE};
    VkFence _immCmdFence{VK_NULL_HANDLE};
};

}
}
}
