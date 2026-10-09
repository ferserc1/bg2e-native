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

#include "detail/ImGuiBackend.hpp"
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Queue.hpp>
#include <bg2e/gpu/vk/WindowSurface.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace bg2e::ui::detail {
namespace {
void check(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error("ImGui Vulkan operation failed: " + std::to_string(result));
}

template<class Concrete, class Abstract>
Concrete& require(Abstract* object, const char* description)
{
    auto* result = dynamic_cast<Concrete*>(object);
    if (!result) throw std::invalid_argument(description);
    return *result;
}

class VulkanImGuiBackend final : public ImGuiBackend {
public:
    void initialize(render::Engine& engine) override
    {
        _production = &engine;
        auto& instance = require<gpu::vk::Instance>(engine.instance(), "Production UI requires a Vulkan instance.");
        _info.Instance = instance.vkInstanceHnd();
        _info.PhysicalDevice = engine.physicalDevice().handle();
        _info.Device = engine.device().handle();
        _info.Queue = engine.command().graphicsQueue();
        _info.QueueFamily = engine.command().graphicsQueueFamily();
        _info.MinImageCount = engine.numImages();
        _info.ImageCount = engine.numImages();
        _format = engine.swapchain().imageFormat();
        // Keep production allocation and engine-registered shutdown behavior.
        _commandPool = engine.command().createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        auto fenceInfo = render::vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        check(vkCreateFence(_info.Device, &fenceInfo, nullptr, &_fence));
        engine.command().allocateCommandBuffer(_commandPool, 1);
        createPool();
        initializeRenderer();
    }

    void initialize(draw::Engine& engine) override
    {
        _draw = &engine;
        auto& instance = require<gpu::vk::Instance>(engine.instance(), "Draw UI requires a Vulkan instance.");
        auto& physical = require<gpu::vk::PhysicalDevice>(engine.physicalDevice(), "Draw UI requires a Vulkan physical device.");
        auto& device = require<gpu::vk::Device>(engine.device(), "Draw UI requires a Vulkan device.");
        auto& queue = require<const gpu::vk::Queue>(&device.graphicsQueue(), "Draw UI requires a Vulkan graphics queue.");
        _info.Instance = instance.vkInstanceHnd();
        _info.PhysicalDevice = physical.handle();
        _info.Device = device.handle();
        _info.Queue = queue.handle();
        _info.QueueFamily = queue.familyIndex();
        readSurfaceConfiguration();
        createPool();
        initializeRenderer();
    }

    void prepareFrame(gpu::CommandBuffer* command, gpu::SurfaceFrame* frame) override
    {
        if (_draw)
        {
            if (!command || !frame || !frame->isValid())
                throw std::logic_error("Draw UI preparation requires an acquired frame and commands.");
            require<gpu::vk::CommandBuffer>(command, "UI commands must use Vulkan.");
            if (command->hasActiveScope()) throw std::logic_error("Prepare UI outside active command scopes.");
            if (_generation != _draw->surface()->generation())
            {
                const auto oldFormat = _format;
                const auto oldCount = _info.ImageCount;
                const auto oldMin = _info.MinImageCount;
                // Read counts/capabilities from the current swapchain, not frame slots.
                readSurfaceConfiguration();
                if (oldFormat != _format || oldCount != _info.ImageCount || oldMin != _info.MinImageCount)
                {
                    _draw->device()->waitIdle();
                    stopRenderer();
                    initializeRenderer();
                }
            }
        }
        ImGui_ImplVulkan_NewFrame();
    }

    void draw(VkCommandBuffer command, VkImageView imageView) override
    {
        if (!_production) throw std::logic_error("Native Vulkan UI draw requires the production engine.");
        using namespace render::vulkan;
        auto attachment = Info::attachmentInfo(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto rendering = Info::renderingInfo(_production->swapchain().extent(), &attachment, nullptr);
        cmdBeginRendering(command, &rendering);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);
        cmdEndRendering(command);
    }

    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        if (!_draw || !frame.isValid() || !frame.colorImage())
            throw std::logic_error("Draw UI requires a valid presentation target.");
        auto& vkCommand = require<gpu::vk::CommandBuffer>(&command, "UI commands must use Vulkan.");
        if (command.hasActiveScope()) throw std::logic_error("UI overlay requires closed scene scopes.");
        command.beginRendering(frame.colorImage()); // Color-only LOAD/STORE, no clear.
        vkCommand.materializeRenderPass();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCommand.handle());
        command.endRendering();
    }

    void shutdown() override
    {
        stopRenderer();
        if (_pool) vkDestroyDescriptorPool(_info.Device, _pool, nullptr);
        if (_fence) vkDestroyFence(_info.Device, _fence, nullptr);
        if (_commandPool) vkDestroyCommandPool(_info.Device, _commandPool, nullptr);
        _pool = VK_NULL_HANDLE;
        _fence = VK_NULL_HANDLE;
        _commandPool = VK_NULL_HANDLE;
        _production = nullptr;
        _draw = nullptr;
    }

private:
    void readSurfaceConfiguration()
    {
        auto& surface = require<gpu::vk::WindowSurface>(_draw->surface(), "Draw Vulkan UI requires a Vulkan window surface.");
        VkSurfaceCapabilitiesKHR capabilities{};
        check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_info.PhysicalDevice, surface.handle(), &capabilities));
        _info.MinImageCount = std::max(2u, capabilities.minImageCount);
        _info.ImageCount = surface.imageCount();
        if (_info.ImageCount < _info.MinImageCount ||
            (capabilities.maxImageCount && _info.MinImageCount > capabilities.maxImageCount))
            throw std::runtime_error("Vulkan swapchain image counts do not satisfy ImGui's minimum of two.");
        _format = gpu::vk::toVkFormat(surface.colorFormat());
        _generation = surface.generation();
    }

    void createPool()
    {
        VkDescriptorPoolSize sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.maxSets = 1000;
        info.poolSizeCount = uint32_t(sizeof(sizes) / sizeof(sizes[0]));
        info.pPoolSizes = sizes;
        check(vkCreateDescriptorPool(_info.Device, &info, nullptr, &_pool));
    }

    void initializeRenderer()
    {
        if (_info.MinImageCount < 2 || _info.ImageCount < _info.MinImageCount)
            throw std::runtime_error("ImGui Vulkan requires at least two swapchain images.");
        _info.ApiVersion = VK_API_VERSION_1_3;
        _info.DescriptorPool = _pool;
        _info.UseDynamicRendering = true;
        _info.CheckVkResultFn = check;
        _info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        auto& rendering = _info.PipelineInfoMain.PipelineRenderingCreateInfo;
        rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &_format; // Backend-owned lifetime.
        _rendererStarted = true;
        if (!ImGui_ImplVulkan_Init(&_info)) throw std::runtime_error("ImGui Vulkan initialization failed.");
    }

    void stopRenderer()
    {
        if (_rendererStarted && ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplVulkan_Shutdown();
        _rendererStarted = false;
    }

    render::Engine* _production = nullptr;
    draw::Engine* _draw = nullptr;
    VkDescriptorPool _pool = VK_NULL_HANDLE;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkFence _fence = VK_NULL_HANDLE;
    ImGui_ImplVulkan_InitInfo _info{};
    VkFormat _format = VK_FORMAT_UNDEFINED;
    uint64_t _generation = 0;
    bool _rendererStarted = false;
};
}

std::unique_ptr<ImGuiBackend> createVulkanImGuiBackend()
{
    return std::make_unique<VulkanImGuiBackend>();
}
}
