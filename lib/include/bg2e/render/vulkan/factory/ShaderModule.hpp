#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>

#include <string>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API ShaderModule
{
public:
    // If the basePath is not specified, the shader will be loaded from the
    // default shader path (see PlatformTools::shaderPath())
    static VkShaderModule loadFromSPV(
        const std::string& fileName,
        VkDevice device,
        const std::string& basePath = ""
    );
};

}
}
}
}

