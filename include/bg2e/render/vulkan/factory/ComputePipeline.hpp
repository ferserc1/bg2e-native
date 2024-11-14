#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API ComputePipeline {
public:
    ComputePipeline(Vulkan * vulkan);
    ~ComputePipeline();

    void setShader(const std::string& fileName, const std::string& entryPoint = "main", const std::string& basePath = "");
    void setShader(VkShaderModule shaderModule, const std::string& entryPoint = "main");

    VkPipeline build(VkPipelineLayout layout);

protected:
    Vulkan * _vulkan;

    VkShaderModule _shaderModule;
    std::string _shaderEntryPoint = "main";
    VkPipelineShaderStageCreateInfo _shaderStageInfo = {};
};

}
}
}
}
