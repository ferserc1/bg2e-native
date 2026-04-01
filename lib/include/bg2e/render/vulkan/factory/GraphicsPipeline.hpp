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
#include <bg2e/render/Engine.hpp>
#include <bg2e/geo/Mesh.hpp>

#include <vector>
#include <set>

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

    void addDynamicState(VkDynamicState state) { _dynamicStates.emplace(state); }
    
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

    // Viewport and scissor are allways dynamic
    std::set<VkDynamicState> _dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    
    std::vector<VkVertexInputBindingDescription> _bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;
};

}
}
}
}

