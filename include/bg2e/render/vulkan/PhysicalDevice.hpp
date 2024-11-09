
#pragma once

#include <bg2e/render/vulkan/common.hpp>
#include <optional>

namespace bg2e {
namespace render {
namespace vulkan {

class Instance;

class PhysicalDevice {
public:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        
        inline bool isComplete()
        {
            return graphics.has_value() && present.has_value();
        }
        
        static QueueFamilyIndices get(VkPhysicalDevice device);
    };

    void create(Instance * instance);
  
protected:
    bool isSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensions(VkPhysicalDevice device);
};

}
}
}
