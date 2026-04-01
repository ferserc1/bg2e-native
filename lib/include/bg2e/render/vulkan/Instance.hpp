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
 
    bool getRequiredLayers(std::vector<const char*>& requiredLayers) const;
    bool getRequiredExtensions(SDL_Window *, std::vector<const char*>& requiredExtensions) const;

protected:
	VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

	bool _enableValidationLayers = false;
    static bool s_debugLayerAvailable;
	std::string _applicationName = "bg2 engine Vulkan Application";

	std::vector<std::string> _availableExtensions;
    std::vector<std::string> _availableLayers;

	
    
    VkResult createDebugMessenger();
    void destroyDebugMessenger();
};

}
}
}
