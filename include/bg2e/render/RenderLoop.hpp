#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API RenderLoop {
public:
    void init(Vulkan * vulkan);

protected:
    
};

}
}
