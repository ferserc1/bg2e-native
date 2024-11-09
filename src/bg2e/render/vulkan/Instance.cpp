#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/base/Log.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

Instance::Instance()
{
    uint32_t layersCount = 0;
    vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layersCount);
    vkEnumerateInstanceLayerProperties(&layersCount, layerProperties.data());
    bg2e_log_debug << "Available Instance Layers:" << bg2e_log_end;
    for (const auto& layer : layerProperties)
    {
        bg2e_log_debug << "\t" << layer.layerName << bg2e_log_end;
        _availableLayers.push_back(layer.layerName);
    }
    
    
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	bg2e_log_debug << "Available Instance Extensions:" << bg2e_log_end;;
	for (const auto& extension : availableExtensions)
    {
		bg2e_log_debug << "\t" << extension.extensionName << bg2e_log_end;
		_availableExtensions.push_back(extension.extensionName);
	}
}

void Instance::create(SDL_Window * sdlWindow)
{
    if (!checkLayerSupport())
    {
        throw std::runtime_error("Instance::create(): missing required instance layers");
    }
    std::vector<std::string> instanceExtensions;
    if (!getRequiredExtensions(sdlWindow, instanceExtensions))
    {
        throw std::runtime_error("Instance::create(): missing required instance extensions");
    }
    
}

bool Instance::checkLayerSupport()
{
    std::vector<const char*> requiredLayers;
    
    if (base::Log::isDebug())
    {
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
    
    for (const char* requiredLayer : requiredLayers)
    {
        bool found = false;
        
        for (const auto& availableLayer : _availableLayers)
        {
            if (requiredLayer == std::string(availableLayer))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            bg2e_log_error << "Error: required layer not present - " << requiredLayer << bg2e_log_end;
            return false;
        }
    }
	return true;
}

bool Instance::getRequiredExtensions(SDL_Window * sdlWindow, std::vector<std::string>& requiredExtensions)
{
    unsigned int sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlExtensionCount, nullptr);
    std::vector<const char*> sdlExtensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlExtensionCount, sdlExtensions.data());
    
    for (auto requiredExtension : sdlExtensions)
    {
        requiredExtensions.push_back(requiredExtension);
    }
    
#if BG2E_IS_MAC
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    if (base::Log::isDebug())
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    // TODO: Check if all the extensions are present
    for (auto& requiredExtension : requiredExtensions)
    {
        bool present = false;
        
        for (const auto& availableExtension : _availableExtensions)
        {
            if (availableExtension == std::string(requiredExtension))
            {
                present = true;
                break;
            }
        }
        
        if (!present)
        {
            bg2e_log_error << "Error: required extension not present - " << requiredExtension << bg2e_log_end;
            return false;
        }
    }
    
    return true;
}

}
}
}
