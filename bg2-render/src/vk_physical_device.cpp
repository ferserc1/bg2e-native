
#include <bg2render/vk_physical_device.hpp>

#include <bg2render/vk_instance.hpp>

#include <set>

namespace bg2render {
    namespace vk {

        PhysicalDevice::PhysicalDevice(Instance * instance, VkPhysicalDevice dev, VkSurfaceKHR surface)
            :_instance(instance)
			,_surface(surface)
            ,_physicalDevice(dev)
        {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies) {
				if (queueFamily.queueCount > 0 &&
					queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					_queueIndices.graphicsFamily = i;
				}

				if (surface != VK_NULL_HANDLE) {
					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
					if (queueFamily.queueCount > 0 && presentSupport) {
						_queueIndices.presentFamily = i;
					}
				}

				if (_queueIndices.isComplete()) {
					break;
				}

				++i;
			}
        }

        PhysicalDevice::~PhysicalDevice() {
        }

		void PhysicalDevice::getProperties(VkPhysicalDeviceProperties& properties) const {
			vkGetPhysicalDeviceProperties(_physicalDevice, &properties);
		}

		void PhysicalDevice::getFeatures(VkPhysicalDeviceFeatures& features) const {
			vkGetPhysicalDeviceFeatures(_physicalDevice, &features);
		}

		bool PhysicalDevice::checkExtensionSupport(const std::vector<const char*>& ext) const {
			std::vector<VkExtensionProperties> supportedExtensions;
			getExtensionProperties(supportedExtensions);
			std::set<std::string> requiredExtensions(ext.begin(), ext.end());
			
			for (const auto& extension : supportedExtensions) {
				requiredExtensions.erase(extension.extensionName);
			}
			return requiredExtensions.empty();
		}

		void PhysicalDevice::getExtensionProperties(std::vector<VkExtensionProperties>& ext) const {
			uint32_t extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);
			ext.resize(extensionCount);
			vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, ext.data());
		}

		const PhysicalDevice::SwapChainSupportDetails& PhysicalDevice::getSwapChainSupport() {
			if (_surface != VK_NULL_HANDLE) {
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &_swapChainSupportDetails.capabilities);

				uint32_t formatCount = 0;
				vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);
				if (formatCount > 0) {
					_swapChainSupportDetails.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, _swapChainSupportDetails.formats.data());
				}

				uint32_t presentModeCount = 0;
				vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
				if (presentModeCount > 0) {
					_swapChainSupportDetails.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, _swapChainSupportDetails.presentModes.data());
				}
			}
			return _swapChainSupportDetails;
		}
    }
}

