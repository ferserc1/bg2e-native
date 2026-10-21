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

#include <bg2e/gpu/GuiRenderer.hpp>
#include <bg2e/gpu/vk/TexturePreview.hpp>
#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Queue.hpp>
#include <bg2e/gpu/vk/WindowSurface.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace bg2e::gpu::vk {
namespace {
void check(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("ImGui Vulkan operation failed: " + std::to_string(result));
    }
}

template <class Concrete, class Abstract>
Concrete& require(Abstract* object, const char* description)
{
    auto* result = dynamic_cast<Concrete*>(object);
    if (!result)
    {
        throw std::invalid_argument(description);
    }
    return *result;
}

class VulkanImGuiBackend final : public GuiRenderer {
  public:

    void initialize(gpu::Instance& instanceObject, gpu::PhysicalDevice& physicalObject,
                    gpu::Device& deviceObject, gpu::WindowSurface& surfaceObject) override
    {
        if (_device || !ImGui::GetCurrentContext() || ImGui::GetIO().BackendRendererUserData)
        {
            throw std::logic_error("GUI renderer requires a fresh bridge and an unbound current ImGui context.");
        }
        _device = &deviceObject;
        _surface = &surfaceObject;
        auto& instance = require<gpu::vk::Instance>(&instanceObject, "Draw UI requires a Vulkan instance.");
        auto& physical = require<gpu::vk::PhysicalDevice>(&physicalObject, "Draw UI requires a Vulkan physical device.");
        auto& device = require<gpu::vk::Device>(&deviceObject, "Draw UI requires a Vulkan device.");
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
        if (!_device || !_rendererStarted)
        {
            throw std::logic_error("Prepare an initialized GUI renderer.");
        }
        if (!command || !frame || !frame->isValid())
        {
            throw std::logic_error("Draw UI preparation requires an acquired frame and commands.");
        }
        require<gpu::vk::CommandBuffer>(command, "UI commands must use Vulkan.");
        if (command->hasActiveScope())
        {
            throw std::logic_error("Prepare UI outside active command scopes.");
        }
        if (_generation != _surface->generation())
        {
            const auto oldFormat = _format;
            const auto oldCount = _info.ImageCount;
            const auto oldMin = _info.MinImageCount;
            // Read counts/capabilities from the current swapchain, not frame slots.
            readSurfaceConfiguration();
            if (oldFormat != _format || oldCount != _info.ImageCount || oldMin != _info.MinImageCount)
            {
                _device->waitIdle();
                stopRenderer();
                initializeRenderer();
            }
        }
        ImGui_ImplVulkan_NewFrame();
    }

    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        if (!_device || !frame.isValid() || !frame.colorImage())
        {
            throw std::logic_error("Draw UI requires a valid presentation target.");
        }
        auto& vkCommand = require<gpu::vk::CommandBuffer>(&command, "UI commands must use Vulkan.");
        if (command.hasActiveScope())
        {
            throw std::logic_error("UI overlay requires closed scene scopes.");
        }
        command.beginRendering(frame.colorImage()); // Color-only LOAD/STORE, no clear.
        vkCommand.materializeRenderPass();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCommand.handle());
        command.endRendering();
    }

    void shutdown() override
    {
        stopRenderer();
        if (_pool)
        {
            vkDestroyDescriptorPool(_info.Device, _pool, nullptr);
        }

        _pool = VK_NULL_HANDLE;
        _device = nullptr;
        _surface = nullptr;
    }

    uint64_t registerTexture(gpu::Image& image, gpu::Sampler& sampler) override
    {
        const auto identifier = gpu::vk::registerTexturePreview(image, sampler);
        _previewIdentifiers.insert(identifier);
        return identifier;
    }

    void removeTexture(uint64_t identifier) override
    {
        if (_previewIdentifiers.erase(identifier))
        {
            gpu::vk::removeTexturePreview(identifier);
        }
    }

    uint64_t textureGeneration() const override
    {
        return _textureGeneration;
    }

  private:
    void readSurfaceConfiguration()
    {
        auto& surface = require<gpu::vk::WindowSurface>(_surface, "Draw Vulkan UI requires a Vulkan window surface.");
        VkSurfaceCapabilitiesKHR capabilities{};
        check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_info.PhysicalDevice, surface.handle(), &capabilities));
        _info.MinImageCount = std::max(2u, capabilities.minImageCount);
        _info.ImageCount = surface.imageCount();
        if (_info.ImageCount < _info.MinImageCount ||
            (capabilities.maxImageCount && _info.MinImageCount > capabilities.maxImageCount))
        {
            throw std::runtime_error("Vulkan swapchain image counts do not satisfy ImGui's minimum of two.");
        }
        _format = gpu::vk::toVkFormat(surface.colorFormat());
        _generation = surface.generation();
    }

    void createPool()
    {
        VkDescriptorPoolSize sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
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
        ++_textureGeneration;
        if (_info.MinImageCount < 2 || _info.ImageCount < _info.MinImageCount)
        {
            throw std::runtime_error("ImGui Vulkan requires at least two swapchain images.");
        }
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
        if (!ImGui_ImplVulkan_Init(&_info))
        {
            throw std::runtime_error("ImGui Vulkan initialization failed.");
        }
    }

    void stopRenderer()
    {
        if (_rendererStarted && ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData)
        {
            for (const auto identifier : _previewIdentifiers)
            {
                gpu::vk::removeTexturePreview(identifier);
            }
            _previewIdentifiers.clear();
            ImGui_ImplVulkan_Shutdown();
        }
        _rendererStarted = false;
    }

    gpu::Device* _device = nullptr;
    gpu::WindowSurface* _surface = nullptr;
    VkDescriptorPool _pool = VK_NULL_HANDLE;
    ImGui_ImplVulkan_InitInfo _info{};
    VkFormat _format = VK_FORMAT_UNDEFINED;
    uint64_t _generation = 0;
    bool _rendererStarted = false;
    uint64_t _textureGeneration = 0;
    std::unordered_set<uint64_t> _previewIdentifiers;
};
}

std::unique_ptr<GuiRenderer> createGuiRenderer()
{
    return std::make_unique<VulkanImGuiBackend>();
}
}
