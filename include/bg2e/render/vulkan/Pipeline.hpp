#ifndef bg2e_render_vulkan_pipeline_hpp
#define bg2e_render_vulkan_pipeline_hpp

#include <bg2e/export.hpp>
#include <bg2e/render/vulkan/PipelineLayout.hpp>
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_EXPORT Pipeline {
public:
    Pipeline();
    Pipeline(const Pipeline*);
    
    inline void setLayout(std::shared_ptr<PipelineLayout> layout) {
        _pipelineLayout = layout;
    }
    
    void build(VulkanAPI *);

    void destroy();

    vk::Pipeline impl() const { return _pipeline;  }

    std::vector<vk::DynamicState> dynamicStates;
    // TODO: pVertexBindingDescriptions
    // TODO: pVertexAttributeDescriptions
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    // TODO: Multisample
    
    
    inline void setViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f)
    {
        _viewport.x = x;
        _viewport.y = y;
        _viewport.width = w;
        _viewport.height = h;
        _viewport.minDepth = minDepth;
        _viewport.maxDepth = maxDepth;
    }

    inline void setViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h, float minDepth = 0.0f, float maxDepth = 1.0f)
    {
        setViewport(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h), minDepth, maxDepth);
    }

    inline void setScissor(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height)
    {
        _scissor.offset.x = offsetX;
        _scissor.offset.y = offsetY;
        _scissor.extent.width = width;
        _scissor.extent.height = height;
    }

    inline void setColorBlendState(
        bool logicOpEnable, 
        vk::LogicOp logicOp, 
        std::array<float, 4>&& blendConstants)
    {
        _colorBlendCreateInfo.logicOpEnable = logicOpEnable;
        _colorBlendCreateInfo.logicOp = logicOp;
        _colorBlendCreateInfo.blendConstants = blendConstants;
    }

    inline void addColorBlendAttachment(
        bool blendEnable,
        vk::BlendFactor srcColorBlendFactor,
        vk::BlendFactor dstColorBlendFactor,
        vk::BlendOp colorBlendOp,
        vk::BlendFactor srcAlphaBlendFactor,
        vk::BlendFactor dstAlphaBlendFactor,
        vk::BlendOp alphaBlendOp,
        vk::ColorComponentFlags colorWriteMask)
    {
        _colorBlendAttachments.push_back({
            blendEnable,
            srcColorBlendFactor,
            dstColorBlendFactor,
            colorBlendOp,
            srcAlphaBlendFactor,
            dstAlphaBlendFactor,
            alphaBlendOp,
            colorWriteMask
            });
    }


protected:
    std::shared_ptr<PipelineLayout> _pipelineLayout;

    vk::Viewport _viewport;
    vk::Rect2D _scissor;
    std::vector<vk::PipelineColorBlendAttachmentState> _colorBlendAttachments;
    vk::PipelineColorBlendStateCreateInfo _colorBlendCreateInfo;


    vk::GraphicsPipelineCreateInfo _createInfo;
    vk::Pipeline _pipeline;
    vk::Device _device;
};

}
}
}

#endif

