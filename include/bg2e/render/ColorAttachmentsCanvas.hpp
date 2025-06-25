//
//  FragmentShaderCanvas.hpp

#pragma once

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>

#include <memory>
#include <string>

namespace bg2e {
namespace render {

class BG2E_API ColorAttachmentsCanvas {
public:
    ColorAttachmentsCanvas(Engine *, std::shared_ptr<ColorAttachments> attachments);
    
    // TODO: Allow to add uniform buffers
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(const std::string& fragmentShader);
    
    void draw(VkCommandBuffer cmd, uint32_t currentFrame, vulkan::FrameResources& frameResources);
    
protected:
    Engine * _engine;
    
    std::shared_ptr<ColorAttachments> _attachments;
    
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
};

}
}
