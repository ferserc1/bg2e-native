#ifndef _bg2_render_vk_device_hpp_
#define _bg2_render_vk_device_hpp_

#include <vulkan/vulkan.h>

#include <bg2render/vk_definitions.hpp>

#include <vector>

namespace bg2render {
    namespace vk {

		class PhysicalDevice;
        class Device {
        public:
			Device(PhysicalDevice * physicalDev, uint32_t deviceTasks);
			virtual ~Device();

			void configureEnabledExtensions(const std::vector<const char*>& ext);
			void configureEnabledLayers(const std::vector<const char*>& layers);

			void create();

            inline VkDevice device() const { return _device; }

			// NOTE: to create the graphicsQueue, you must specify the vkDeviceTaskRender in the constructor
			inline VkQueue graphicsQueue() const { return _graphicsQueue; }

        protected:
			PhysicalDevice* _physicalDevice;
			uint32_t _deviceTasks;
			std::vector<const char*> _enabledExtensions;
			std::vector<const char*> _enabledLayers;

            VkDevice _device = VK_NULL_HANDLE;
			VkQueue _graphicsQueue = VK_NULL_HANDLE;
        };
    }
}

#endif
