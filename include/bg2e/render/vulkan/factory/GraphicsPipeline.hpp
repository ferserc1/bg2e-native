
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/geo/Mesh.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API GraphicsPipeline {
public:
    GraphicsPipeline(Engine * engine);
    ~GraphicsPipeline();
    
    void addShader(const std::string& fileName, VkShaderStageFlagBits stage, const std::string& entryPoint = "main", const std::string& basePath = "");
    void addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const std::string& entryPoint = "main");
    void clearShaders();
    
    void addInputBindingDescription(VkVertexInputBindingDescription desc);
    void addInputAttributeDescription(VkVertexInputAttributeDescription desc);
    void setInputBindingDescription(VkVertexInputBindingDescription desc);
    void setInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& desc);
    void clearInputBindingDescriptions();
    void clearInputAttributeDescriptions();
    
    // Template function to add both input binding and attribute descriptions depending on the type of mesh
    // TODO: Check if this is working on Visual Studio/Windows
    template <typename MeshT>
    void setInputState()
    {
        setInputBindingDescription(MeshT::bindingDescription());
        setInputAttributeDescriptions(MeshT::attributeDescriptions());
    }

    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode, float lineWidth = 1.0f);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    void enableMultisample();
    void disableMultisample();
    void setColorAttachmentFormat(VkFormat format, uint32_t viewMask = 0);
    void setColorAttachmentFormat(const std::vector<VkFormat>& format, uint32_t viewMask = 0);
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
    
    void reset();
    
protected:
    Engine * _engine;

    VkPipelineRenderingCreateInfo _renderInfo = {};
    std::vector<VkFormat> _colorAttachmentformat;
    
    struct ShaderData {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
        std::string entryPoint;
    };
    
    std::vector<ShaderData> _shaders;
    
    std::vector<VkVertexInputBindingDescription> _bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;
};

}
}
}
}

