
#include <bg2render/vk_physical_device.hpp>

#include <bg2render/vk_instance.hpp>

namespace bg2render {
    namespace vk {

        PhysicalDevice::PhysicalDevice(Instance * instance, VkPhysicalDevice dev, VkSurfaceKHR surface)
            :_instance(instance)
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
    }
}

