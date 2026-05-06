/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <vector>
#include <string>

namespace bg2e {
namespace gpu {
namespace vk {

class Instance : public gpu::Instance {
public:
    Instance();

    // gpu::Instance interface
    void setApplicationName(const std::string& name) override;
    const std::string& applicationName() const override;
    void enableDebugMode(bool value) override;
    bool debugModeEnabled() const override;

    void create(SDL_Window* window) override;
    void create() override;
    void cleanup() override;

    // Vulkan-specific accessor (not part of abstract interface)
    VkInstance vkInstanceHnd() const { return _instance; }

    // Layer and extension queries — used by Device during creation
    bool getRequiredLayers(std::vector<const char*>& requiredLayers) const;
    bool getRequiredExtensions(SDL_Window* window, std::vector<const char*>& requiredExtensions) const;
    bool getRequiredExtensions(std::vector<const char*>& requiredExtensions) const;

private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

    bool _debugMode = false;
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
