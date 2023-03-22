
#ifndef bg2e_render_vulkan_tools_hpp
#define bg2e_render_vulkan_tools_hpp

#include <vulkan/vulkan.hpp>


#include <vector>
#include <string>

namespace bg2e {
namespace render {
namespace vulkan {

bool checkValidationLayerSupport();

std::vector<const char*> getRequiredExtensions(bool validationLayersSupport);

vk::Instance createInstance(vk::ApplicationInfo& appInfo, bool validationLayersSupport);

VkDebugUtilsMessengerEXT setupDebugMessenger(vk::Instance instance);

void destroyDebugMessenger(vk::Instance instance, VkDebugUtilsMessengerEXT debugMessenger);

}
}
}

#endif

