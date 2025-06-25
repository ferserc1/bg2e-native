//
//  ColorAttachmentsCanvas.cpp

#include <bg2e/render/ColorAttachmentsCanvas.hpp>

namespace bg2e::render {

ColorAttachmentsCanvas::ColorAttachmentsCanvas(Engine * engine, std::shared_ptr<ColorAttachments> attachments)
    :_engine{ engine }
    ,_attachments(attachments)
{

}
    
void ColorAttachmentsCanvas::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<float>(_attachments->size()) }
    });
}

void ColorAttachmentsCanvas::build(const std::string& fragmentShader)
{

}

void ColorAttachmentsCanvas::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources
) {

}

}
