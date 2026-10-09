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
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/SurfaceFrame.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <stdexcept>

namespace bg2e::gpu::vk {
namespace {
void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": Vulkan result " + std::to_string(result));
}
}

Queue::Queue(VkQueue queue, uint32_t family) : _queue(queue), _familyIndex(family) {}
uint32_t Queue::familyIndex() const { return _familyIndex; }
bool Queue::isValid() const { return _queue != VK_NULL_HANDLE; }
VkQueue Queue::handle() const { return _queue; }

void Queue::initCommandPool(VkDevice device, vk::Device* gpuDevice)
{
    _device = device;
    _gpuDevice = gpuDevice;
    _submissions = gpuDevice->submissionState();
    auto pool = std::make_shared<CommandPoolState>();
    pool->device = device;
    auto info = Info::commandPoolCreateInfo(_familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    check(vkCreateCommandPool(device, &info, nullptr, &_commandPool), "vkCreateCommandPool");
    pool->pool = _commandPool;
    _poolStates.push_back(std::move(pool));
}

void Queue::destroyCommandPool()
{
    for (const auto& pool : _poolStates) pool->cleanup();
    _poolStates.clear();
    _commandPool = VK_NULL_HANDLE;
    _device = VK_NULL_HANDLE;
    _gpuDevice = nullptr;
    _queue = VK_NULL_HANDLE;
}

std::shared_ptr<gpu::CommandBuffer> Queue::createCommandBuffer(const std::string& debugName) const
{
    if (!_submissions) throw std::logic_error("Vulkan queue has no device");
    auto admission = _submissions->admit();
    // A live command has an exclusive pool: recording may occur on other threads.
    std::shared_ptr<CommandPoolState> pool;
    for (const auto& candidate : _poolStates)
        if (candidate.use_count() == 1) { pool = candidate; break; }
    if (!pool) {
        pool = std::make_shared<CommandPoolState>();
        pool->device = _device;
        auto info = Info::commandPoolCreateInfo(_familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        check(vkCreateCommandPool(_device, &info, nullptr, &pool->pool), "vkCreateCommandPool");
        _poolStates.push_back(pool);
    }
    std::unique_lock poolLock(pool->mutex);
    if (!pool->pool) throw std::logic_error("Vulkan command pool is closed");
    auto allocation = std::make_shared<CommandAllocation>();
    allocation->pool = pool;
    auto info = Info::commandBufferAllocateInfo(pool->pool, 1);
    check(vkAllocateCommandBuffers(_device, &info, &allocation->command), "vkAllocateCommandBuffers");
    poolLock.unlock();
    auto command = std::make_shared<vk::CommandBuffer>(_gpuDevice, allocation->command, pool->pool);
    command->_allocation = allocation;
    command->_originQueue = _queue;
    if (!debugName.empty() && setDebugUtilsObjectName) {
        VkDebugUtilsObjectNameInfoEXT name{};
        name.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        name.objectHandle = reinterpret_cast<uint64_t>(allocation->command);
        name.pObjectName = debugName.c_str();
        setDebugUtilsObjectName(_device, &name);
    }
    return command;
}

void Queue::submit(gpu::CommandBuffer* command) const
{
    if (!_submissions) throw std::logic_error("Vulkan queue has no device");
    auto admission = _submissions->admit();
    auto* cmd = dynamic_cast<vk::CommandBuffer*>(command);
    if (!cmd || cmd->_device != _gpuDevice || cmd->_originQueue != _queue || !cmd->_allocation ||
        !cmd->_allocation->pool->pool || cmd->_allocation->pool->device != _device)
        throw std::invalid_argument("Vulkan command buffer belongs to another queue/device");
    if (cmd->_recording || !cmd->_executable)
        throw std::logic_error("Vulkan command buffer is not executable");
    if (cmd->_completion && !cmd->_completion->completed())
        throw std::logic_error("Vulkan command buffer is still in flight");

    auto* frame = cmd->presentFrame();
    const bool presenting = frame && frame->swapchain() != VK_NULL_HANDLE;
    if (presenting && frame->hasSubmissions())
        throw std::logic_error("Vulkan acquired frame has already been submitted for presentation");
    VkFence fence = presenting ? frame->inFlightFence() : VK_NULL_HANDLE;
    std::shared_ptr<VkFence> ownedFence;
    if (!presenting) {
        auto info = Info::fenceCreateInfo();
        check(vkCreateFence(_device, &info, nullptr, &fence), "vkCreateFence");
        ownedFence = std::shared_ptr<VkFence>(new VkFence(fence), [device = _device](VkFence* value) {
            vkDestroyFence(device, *value, nullptr); delete value;
        });
    }
    auto record = std::make_shared<detail::CompletionRecord>(
        [device = _device, fence, ownedFence, allocation = cmd->_allocation] {
            auto result = vkGetFenceStatus(device, fence);
            if (result == VK_NOT_READY) return false;
            check(result, "vkGetFenceStatus"); return true;
        },
        [device = _device, fence, ownedFence, allocation = cmd->_allocation] {
            check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
        });
    auto info = Info::commandBufferSubmitInfo(cmd->handle());
    try {
        _submissions->track(record);
        cmd->_completion = record;
        if (cmd->_submissionFrame) cmd->_submissionFrame->trackSubmission(record);
        if (presenting) {
            check(vkResetFences(_device, 1, &fence), "vkResetFences");
            auto wait = Info::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame->imageAvailable());
            auto signal = Info::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame->renderFinished());
            auto submit = Info::submitInfo(&info, &signal, &wait);
            check(queueSubmit2(_queue, 1, &submit, fence), "vkQueueSubmit2");
        } else {
            auto submit = Info::submitInfo(&info, nullptr, nullptr);
            check(queueSubmit2(_queue, 1, &submit, fence), "vkQueueSubmit2");
        }
    } catch (...) {
        _submissions->cancel(record, std::current_exception());
        throw;
    }
    cmd->_executable = false;
    // Presentation is in the same admission transaction as submission.
    if (presenting) {
        auto semaphore = frame->renderFinished();
        auto swapchain = frame->swapchain();
        auto index = frame->imageIndex();
        auto info = Info::presentInfo(swapchain, semaphore, index);
        auto result = queuePresent(_queue, &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            frame->requestRecreate();
        else check(result, "vkQueuePresentKHR");
    }
}

}
