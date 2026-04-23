/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/base/Log.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

bool Instance::s_debugLayerAvailable = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL bg2e_mainDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }
    
    return VK_FALSE;
}

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
    std::vector<const char*> requiredLayers;
    if (base::Log::isDebug())
    {
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
    if (!Instance::getRequiredLayers(requiredLayers))
    {
        throw std::runtime_error("Instance::create(): missing required instance layers");
    }
    std::vector<const char*> instanceExtensions;
    if (!Instance::getRequiredExtensions(sdlWindow, instanceExtensions))
    {
        throw std::runtime_error("Instance::create(): missing required instance extensions");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = _applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "bg2 engine - native";
    appInfo.engineVersion = VK_MAKE_VERSION(2, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = uint32_t(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = uint32_t(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    
#ifdef BG2E_IS_MAC
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    
    auto debugCreateInfo = Info::debugMessengerCreateInfo(bg2e_mainDebugCallback);
    if (base::Log::isDebug() && Instance::s_debugLayerAvailable)
    {
        createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
    
    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &_instance));
    
    if (base::Log::isDebug() && Instance::s_debugLayerAvailable)
    {
        VK_ASSERT(createDebugMessenger());
    }
}

void Instance::cleanup()
{
    if (base::Log::isDebug() && Instance::s_debugLayerAvailable)
    {
        destroyDebugMessenger();
    }
    
    vkDestroyInstance(_instance, nullptr);
}

bool Instance::getRequiredLayers(std::vector<const char*>& requiredLayers) const
{
    Instance::s_debugLayerAvailable = false;
    for (const char* requiredLayer : requiredLayers)
    {
        bool found = false;
        
        for (const auto& availableLayer : _availableLayers)
        {
            if (availableLayer == "VK_LAYER_KHRONOS_validation")
            {
                Instance::s_debugLayerAvailable = true;
            }

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

    if (base::Log::isDebug() && !Instance::s_debugLayerAvailable)
    {
        bg2e_log_warning << "Warning: VK_LAYER_KHRONOS_validation layer not present" << bg2e_log_end;
    }
    else
    {
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
	return true;
}

bool Instance::getRequiredExtensions(SDL_Window * sdlWindow, std::vector<const char*>& requiredExtensions) const
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

    if (base::Log::isDebug() && Instance::s_debugLayerAvailable)
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
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

VkResult Instance::createDebugMessenger()
{
    auto debugCreateInfo = Info::debugMessengerCreateInfo(bg2e_mainDebugCallback);
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(_instance, &debugCreateInfo, nullptr, &_debugMessenger);
    }
    return VK_SUCCESS;
}

void Instance::destroyDebugMessenger()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(_instance, _debugMessenger, nullptr);
    }
}
    
}
}
}
