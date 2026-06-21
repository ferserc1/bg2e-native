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

#include <bg2e/gpu/vk/Queue.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/SurfaceFrame.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

Queue::Queue(VkQueue queue, uint32_t family)
    : _queue(queue), _familyIndex(family)
{
}

uint32_t Queue::familyIndex() const
{
    return _familyIndex;
}

bool Queue::isValid() const
{
    return _queue != VK_NULL_HANDLE;
}

VkQueue Queue::handle() const
{
    return _queue;
}

void Queue::initCommandPool(VkDevice device, vk::Device* gpuDevice)
{
    _device = device;
    _gpuDevice = gpuDevice;

    auto poolInfo = Info::commandPoolCreateInfo(
        _familyIndex,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    VK_ASSERT(vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool));
}

void Queue::destroyCommandPool()
{
    if (_commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(_device, _commandPool, nullptr);
        _commandPool = VK_NULL_HANDLE;
    }
    _device = VK_NULL_HANDLE;
    _gpuDevice = nullptr;
}

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& debugName) const
{
    if (_commandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("vk::Queue::createCommandBuffer: command pool not initialized");
    }

    auto allocInfo = Info::commandBufferAllocateInfo(_commandPool, 1);
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_ASSERT(vkAllocateCommandBuffers(_device, &allocInfo, &cmd));

    if (base::Log::isDebug() && !debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(cmd);
        nameInfo.pObjectName = debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
    }

    return std::make_shared<vk::CommandBuffer>(_gpuDevice, cmd, _commandPool);
}

void Queue::submit(gpu::CommandBuffer* cmd) const
{
    auto* vkCmd = dynamic_cast<vk::CommandBuffer*>(cmd);
    if (!vkCmd)
    {
        throw std::runtime_error("vk::Queue::submit: not a vk::CommandBuffer");
    }

    auto* frame = vkCmd->presentFrame();
    auto cmdInfo = Info::commandBufferSubmitInfo(vkCmd->handle());

    if (frame)
    {
        VkSemaphoreSubmitInfo wait = Info::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame->imageAvailable());
        VkSemaphoreSubmitInfo signal = Info::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame->renderFinished());
        auto submit = Info::submitInfo(&cmdInfo, &signal, &wait);
        auto fence = frame->inFlightFence();
        vkResetFences(_device, 1, &fence);
        VK_ASSERT(queueSubmit2(_queue, 1, &submit, fence));

        VkSemaphore    waitSem = frame->renderFinished();
        VkSwapchainKHR sc      = frame->swapchain();
        uint32_t       idx     = frame->imageIndex();
        VkPresentInfoKHR present = Info::presentInfo(sc, waitSem, idx);
        queuePresent(_queue, &present);
    }
    else
    {
        auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);
        VK_ASSERT(queueSubmit2(_queue, 1, &submit, VK_NULL_HANDLE));
    }
}

}
}
}
