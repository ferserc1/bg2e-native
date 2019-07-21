
#include <bg2render/vk_device.hpp>

#include <bg2render/vk_physical_device.hpp>

#include <stdexcept>
#include <set>

namespace bg2render {
    namespace vk {

		Device::Device(PhysicalDevice* physicalDev, uint32_t deviceTasks)
			:_physicalDevice(physicalDev)
			,_deviceTasks(deviceTasks)
		{
		}

		Device::~Device() {
			if (_device != VK_NULL_HANDLE) {
				vkDestroyDevice(_device, nullptr);
			}
		}

		void Device::configureEnabledExtensions(const std::vector<const char*>& ext) {
			for (auto& e : ext) {
				_enabledExtensions.push_back(e);
			}
		}

		void Device::configureEnabledLayers(const std::vector<const char*>& layers) {
			for (auto l : layers) {
				_enabledLayers.push_back(l);
			}
		}

		void Device::create() {
			std::set<uint32_t> uniqueQueueIndices;
			if (_deviceTasks & kDeviceTaskRender) {
				uniqueQueueIndices.insert(_physicalDevice->queueIndices().graphicsFamily);
			}
			if (_deviceTasks & kDeviceTaskPresent) {
				uniqueQueueIndices.insert(_physicalDevice->queueIndices().presentFamily);
			}

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			for (auto queueIndex : uniqueQueueIndices) {
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueIndex;
				queueCreateInfo.queueCount = 1;
				float queuePriority = 1.0f;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures = {};

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pEnabledFeatures = &deviceFeatures;

			if (_enabledExtensions.size() > 0) {
				createInfo.ppEnabledExtensionNames = _enabledExtensions.data();
			}
			createInfo.enabledExtensionCount = static_cast<uint32_t>(_enabledExtensions.size());

			if (_enabledLayers.size() > 0) {
				createInfo.ppEnabledLayerNames = _enabledLayers.data();
			}
			createInfo.enabledLayerCount = static_cast<uint32_t>(_enabledLayers.size());

			if (vkCreateDevice(_physicalDevice->physicalDevice(), &createInfo, nullptr, &_device) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Vulkan logical device");
			}

			if (_deviceTasks & kDeviceTaskRender) {
				vkGetDeviceQueue(_device, _physicalDevice->queueIndices().graphicsFamily, 0, &_graphicsQueue);
			}
			if (_deviceTasks & kDeviceTaskPresent) {
				vkGetDeviceQueue(_device, _physicalDevice->queueIndices().presentFamily, 0, &_presentQueue);
			}
		}
    }
}
