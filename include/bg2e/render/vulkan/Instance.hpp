#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include <SDL2/SDL.h>


#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Instance {
public:
	Instance();

	inline void enableValidationLayers(bool value) { _enableValidationLayers = value; }
	inline bool isValidationLayersEnabled() const { return _enableValidationLayers; }
	inline void setApplicationName(const std::string& name) { _applicationName = name; }
	inline const std::string& applicationName() const { return _applicationName; }

    void create(SDL_Window *);
    
    void cleanup();

	inline VkInstance handle() const { return _instance; }

protected:
	VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

	bool _enableValidationLayers = false;
	std::string _applicationName = "bg2 engine Vulkan Application";

	std::vector<std::string> _availableExtensions;
    std::vector<std::string> _availableLayers;

	bool getRequiredLayers(std::vector<const char*>& requiredLayers);
    bool getRequiredExtensions(SDL_Window *, std::vector<const char*>& requiredExtensions);
    
    VkResult createDebugMessenger();
    void destroyDebugMessenger();
};

}
}
}
