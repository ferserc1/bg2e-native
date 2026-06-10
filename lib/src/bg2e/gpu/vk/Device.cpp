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

#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/ShaderModule.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/GraphicsPipeline.hpp>
#include <bg2e/gpu/vk/ComputePipeline.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/base/Log.hpp>

#include <set>
#include <vector>
#include <memory>

namespace bg2e {
namespace gpu {
namespace vk {

void Device::create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice, gpu::Surface* surface)
{
    auto* vkPhysDevice = dynamic_cast<vk::PhysicalDevice*>(physicalDevice);
    bool offscreen = surface->isOffscreen();
    auto indices = vkPhysDevice->queueFamilyIndices();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphics.value()
    };

    if (!offscreen)
    {
        uniqueQueueFamilies.insert(indices.present.value());
    }

    if (indices.transfer.has_value())
    {
        uniqueQueueFamilies.insert(indices.transfer.value());
    }

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // --- FEATURE CHAIN ---

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features{};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Features.pNext = &dynamicRenderingFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.pNext = &synchronization2Features;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.pNext = &rayQueryFeatures;

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.pNext = &rayTracingPipelineFeatures;

    VkPhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.pNext = &vulkan12Features;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &vulkan11Features;

    // --- QUERY ---
    vkGetPhysicalDeviceFeatures2(vkPhysDevice->handle(), &features2);

    // --- base validation ---
    if (!dynamicRenderingFeatures.dynamicRendering)
    {
        throw std::runtime_error("Dynamic rendering not supported");
    }

    if (!synchronization2Features.synchronization2)
    {
        throw std::runtime_error("Synchronization 2 not supported");
    }

    // --- Base activation ---
    features2.features.samplerAnisotropy = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan11Features.multiview = VK_TRUE;

    // --- RT optional ---
    std::vector<const char*> rtExtensions;

    bool enableRT =
        vkPhysDevice->properties()->rayTracing.fullSupported() &&
        accelerationStructureFeatures.accelerationStructure &&
        rayQueryFeatures.rayQuery &&
        rayTracingPipelineFeatures.rayTracingPipeline;

    if (enableRT)
    {
        const std::vector<const char*> optionalRTExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        };

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(vkPhysDevice->handle(), nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(vkPhysDevice->handle(), nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> availableExtensionNames;
        for (const auto& ext : availableExtensions)
        {
            availableExtensionNames.insert(ext.extensionName);
        }

        for (const auto* ext : optionalRTExtensions)
        {
            if (availableExtensionNames.count(ext))
            {
                rtExtensions.push_back(ext);
            }
        }
    }

    // --- DEVICE CREATE ---
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
    createInfo.pNext = &features2;

    auto deviceExtensions = PhysicalDevice::getRequiredDeviceExtensions(offscreen);
    std::vector<const char*> allExtensions = deviceExtensions;
    allExtensions.insert(allExtensions.end(), rtExtensions.begin(), rtExtensions.end());

    createInfo.enabledExtensionCount = uint32_t(allExtensions.size());
    createInfo.ppEnabledExtensionNames = allExtensions.data();

    // Validation layers for device
    std::vector<const char*> requiredLayers;
    if (base::Log::isDebug())
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const auto& layer : availableLayers)
        {
            if (std::string(layer.layerName) == "VK_LAYER_KHRONOS_validation")
            {
                requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
                break;
            }
        }
    }

    if (!requiredLayers.empty())
    {
        createInfo.enabledLayerCount = uint32_t(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }

    VK_ASSERT(vkCreateDevice(vkPhysDevice->handle(), &createInfo, nullptr, &_device));

    // Load device-level extension function pointers
    loadDeviceExtensions(*vkPhysDevice, _device, offscreen);

    // Retrieve queues
    VkQueue gfxQueue;
    vkGetDeviceQueue(_device, indices.graphics.value(), 0, &gfxQueue);
    _graphicsQueue = vk::Queue(gfxQueue, indices.graphics.value());

    if (!offscreen)
    {
        VkQueue presentQueue;
        vkGetDeviceQueue(_device, indices.present.value(), 0, &presentQueue);
        _presentQueue = vk::Queue(presentQueue, indices.present.value());
    }

    VkQueue transferQueue;
    vkGetDeviceQueue(_device, indices.transfer.value(), 0, &transferQueue);
    _transferQueue = vk::Queue(transferQueue, indices.transfer.value());

    // Initialize command pools for each queue
    _graphicsQueue.initCommandPool(_device, this);
    if (!offscreen) _presentQueue.initCommandPool(_device, this);
    _transferQueue.initCommandPool(_device, this);

    // Immediate submit resources
    {
        auto poolInfo = Info::commandPoolCreateInfo(
            indices.graphics.value(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VK_ASSERT(vkCreateCommandPool(_device, &poolInfo, nullptr, &_immediateCmdPool));

        auto allocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool, 1);
        VK_ASSERT(vkAllocateCommandBuffers(_device, &allocInfo, &_immediateCmdBuffer));

        auto fenceInfo = Info::fenceCreateInfo(0);
        VK_ASSERT(vkCreateFence(_device, &fenceInfo, nullptr, &_immediateCmdFence));
    }

    // --- VMA allocator ---
    auto* vkInst = dynamic_cast<vk::Instance*>(instance);

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = vkPhysDevice->handle();
    allocatorInfo.device         = _device;
    allocatorInfo.instance       = vkInst->vkInstanceHnd();
    allocatorInfo.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_ASSERT(vmaCreateAllocator(&allocatorInfo, &_allocator));

    if (surface)
    {
        surface->createRenderTarget(this, physicalDevice);
    }
}

void Device::cleanup()
{
    _graphicsQueue.destroyCommandPool();
    _presentQueue.destroyCommandPool();
    _transferQueue.destroyCommandPool();

    if (_immediateCmdFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(_device, _immediateCmdFence, nullptr);
        _immediateCmdFence = VK_NULL_HANDLE;
    }
    if (_immediateCmdPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(_device, _immediateCmdPool, nullptr);
        _immediateCmdPool = VK_NULL_HANDLE;
        _immediateCmdBuffer = VK_NULL_HANDLE;
    }

    if (_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(_allocator);
        _allocator = VK_NULL_HANDLE;
    }
    if (_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }
}

void Device::waitIdle()
{
    vkDeviceWaitIdle(_device);
}

bool Device::isValid() const
{
    return _device != VK_NULL_HANDLE;
}

const gpu::Queue& Device::graphicsQueue() const
{
    return _graphicsQueue;
}

const gpu::Queue& Device::presentQueue() const
{
    return _presentQueue;
}

const gpu::Queue& Device::transferQueue() const
{
    return _transferQueue;
}

std::unique_ptr<gpu::ShaderModule> Device::createShaderModule(const gpu::ShaderModuleDescription& description)
{
    return std::make_unique<vk::ShaderModule>(_device, description);
}

std::unique_ptr<gpu::PipelineLayout> Device::createPipelineLayout(const gpu::PipelineLayoutDescription& description)
{
    return std::make_unique<vk::PipelineLayout>(_device, description);
}

std::unique_ptr<gpu::GraphicsPipeline> Device::createGraphicsPipeline(const gpu::GraphicsPipelineDescription& description)
{
    return std::make_unique<vk::GraphicsPipeline>(_device, description);
}

std::unique_ptr<gpu::ComputePipeline> Device::createComputePipeline(const gpu::ComputePipelineDescription& description)
{
    return std::make_unique<vk::ComputePipeline>(_device, description);
}

void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&& function)
{
    VK_ASSERT(vkResetFences(_device, 1, &_immediateCmdFence));
    VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

    vk::CommandBuffer wrapper(this, _immediateCmdBuffer, _immediateCmdPool);

    wrapper.begin();
    function(&wrapper);
    wrapper.end();

    auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);
    VK_ASSERT(queueSubmit2(_graphicsQueue.handle(), 1, &submit, _immediateCmdFence));

    VK_ASSERT(vkWaitForFences(_device, 1, &_immediateCmdFence, VK_TRUE, UINT64_MAX));
}

}
}
}
