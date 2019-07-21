
#ifndef _bg2_render_vk_physical_device_hpp_
#define _bg2_render_vk_physical_device_hpp_

#include <vulkan/vulkan.h>

#include <vector>

namespace bg2render {
    namespace vk {

        class Instance;
        class PhysicalDevice {
        public:
			struct QueueFamilyIndices {
				int32_t graphicsFamily = -1;
				int32_t presentFamily = -1;

				inline bool isComplete() const {
					return graphicsFamily != -1 &&
							presentFamily != -1;
				}
			};

            PhysicalDevice(Instance * instance, VkPhysicalDevice dev, VkSurfaceKHR surface = VK_NULL_HANDLE);
            virtual ~PhysicalDevice();

			void getProperties(VkPhysicalDeviceProperties& properties) const;
			void getFeatures(VkPhysicalDeviceFeatures& features) const;

            inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }

			inline const QueueFamilyIndices& queueIndices() const { return _queueIndices; }

        protected:
            Instance * _instance = nullptr;

            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
			QueueFamilyIndices _queueIndices;
        };

    }
}

#endif
