
#include <bg2render/vk_physical_device.hpp>

#include <bg2render/vk_instance.hpp>

namespace bg2render {
    namespace vk {

        PhysicalDevice::PhysicalDevice(Instance * instance, VkPhysicalDevice dev)
            :_instance(instance)
            ,_physicalDevice(dev)
        {
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

