//
//  GPUSelectionDialog.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>

#include <string>
#include <memory>

namespace bg2e {
namespace app {

class BG2E_API GPUSelectionDialog {
public:
    GPUSelectionDialog(const std::string & appId);
    
    std::shared_ptr<render::vulkan::PhysicalDeviceProperties> run();
    
protected:

    std::string _appId;
    
    std::shared_ptr<render::vulkan::PhysicalDeviceProperties> _selectedDevice;
};

}
}
