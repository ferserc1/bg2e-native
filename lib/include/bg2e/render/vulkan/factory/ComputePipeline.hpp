#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API ComputePipeline {
public:
    ComputePipeline(Engine * engine);
    ~ComputePipeline();

    void setShader(const std::string& fileName, const std::string& entryPoint = "main", const std::string& basePath = "");
    void setShader(VkShaderModule shaderModule, const std::string& entryPoint = "main");

    VkPipeline build(VkPipelineLayout layout);

protected:
    Engine * _engine;

    VkShaderModule _shaderModule;
    std::string _shaderEntryPoint = "main";
    VkPipelineShaderStageCreateInfo _shaderStageInfo = {};
};

}
}
}
}
