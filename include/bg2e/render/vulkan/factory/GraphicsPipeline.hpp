
#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API GraphicsPipeline {
public:
    GraphicsPipeline(Vulkan * vulkan);
    ~GraphicsPipeline();
    
    void addShader(const std::string& fileName, VkShaderStageFlagBits stage, const std::string& entryPoint = "main", const std::string& basePath = "");
    void addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const std::string& entryPoint = "main");
    void clearShaders();

    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode, float lineWidth = 1.0f);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    void disableMultisample();
    void setColorAttachmentFormat(VkFormat format, uint32_t viewMask = 0);
    void setDepthFormat(VkFormat format);
    void disableDepthtest();
    void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();
    
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    
    VkPipeline build(VkPipelineLayout layout);
    
protected:
    Vulkan * _vulkan;

    VkPipelineRenderingCreateInfo _renderInfo = {};
    VkFormat _colorAttachmentformat;
    
    struct ShaderData {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
        std::string entryPoint;
    };
    
    std::vector<ShaderData> _shaders;
};

}
}
}
}

