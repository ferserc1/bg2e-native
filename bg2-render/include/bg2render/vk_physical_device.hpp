
#ifndef _bg2_render_vk_physical_device_hpp_
#define _bg2_render_vk_physical_device_hpp_

#include <vulkan/vulkan.h>

#include <vector>

namespace bg2render {
    namespace vk {

        class Instance;
        class PhysicalDevice {
        public:
            PhysicalDevice(Instance * instance, VkPhysicalDevice dev);
            virtual ~PhysicalDevice();


            inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }

        protected:
            Instance * _instance = nullptr;

            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
        };

    }
}

#endif
