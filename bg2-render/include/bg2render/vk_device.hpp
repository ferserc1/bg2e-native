#ifndef _bg2_render_vk_device_hpp_
#define _bg2_render_vk_device_hpp_

#include <vulkan/vulkan.h>

namespace bg2render {
    namespace vk {

        class Device {
        public:

            inline VkDevice device() const { return _device; }

        protected:
            VkDevice _device;
        };
    }
}

#endif
