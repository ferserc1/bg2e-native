
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Instance.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

PhysicalDevice::QueueFamilyIndices PhysicalDevice::QueueFamilyIndices::get(VkPhysicalDevice device)
{
    QueueFamilyIndices result;
    
    return result;
}

void PhysicalDevice::create(Instance * instance)
{
}

bool PhysicalDevice::isSuitable(VkPhysicalDevice device)
{
    return false;
}

bool PhysicalDevice::checkDeviceExtensions(VkPhysicalDevice device)
{
    return false;
}

}
}
}
