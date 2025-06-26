//
//  ColorAttachmentsCanvas.cpp

#include <bg2e/render/ColorAttachmentsCanvas.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/macros/all.hpp>

namespace bg2e::render {

ColorAttachmentsCanvas::ColorAttachmentsCanvas(Engine * engine, std::shared_ptr<ColorAttachments> attachments)
    :_engine{ engine }
    ,_attachments(attachments)
{

}
    
void ColorAttachmentsCanvas::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<float>(_attachments->attachmentFormats().size()) }
    });
}

void ColorAttachmentsCanvas::build(
    const std::string& fragmentShader,
    VkFormat targetImageFormat,
    VkSampleCountFlagBits sampleCount
) {
    vulkan::factory::GraphicsPipeline plFactory(_engine);
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    vulkan::factory::PipelineLayout plLayoutFactory(_engine);
        
    plFactory.addShader("color_attachment_canvas.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    for (size_t binding = 0; binding < _attachments->images().size(); ++binding)
    {
        dsLayoutFactory.addBinding(static_cast<uint32_t>(binding), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    _attachmentsDSLayout = dsLayoutFactory.build(_engine->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT);
    
    plLayoutFactory.addDescriptorSetLayout(_attachmentsDSLayout);
    _pipelineLayout = plLayoutFactory.build();
    
    plFactory.disableDepthtest();
    sampleCount != VK_SAMPLE_COUNT_1_BIT ? plFactory.enableMultisample() : plFactory.disableMultisample();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.setColorAttachmentFormat(targetImageFormat);
    _pipeline = plFactory.build(_pipelineLayout);
    
    vulkan::factory::Sampler samplerFactory(_engine);
    _attSampler = samplerFactory.build();
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _attachmentsDSLayout, nullptr);
        vkDestroySampler(dev, _attSampler, nullptr);
    });

}

void ColorAttachmentsCanvas::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image * targetImage,
    vulkan::FrameResources& frameResources
) {
    for (auto &image : _attachments->images())
    {
        vulkan::Image::cmdTransitionImage(
            cmd,
            image->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    }
    VkClearColorValue clearValue { { 0.0f, 0.0f, 0.0f, 1.0f } };
    vulkan::macros::cmdClearImagesAndBeginRendering(cmd, { targetImage }, clearValue);
    
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, targetImage->extent2D());
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    
    auto attachmentsDS = frameResources.newDescriptorSet(_attachmentsDSLayout);
    attachmentsDS->beginUpdate();
    for (size_t binding = 0; binding < _attachments->images().size(); ++binding)
    {
        auto image = _attachments->imageWithIndex(static_cast<uint32_t>(binding));
        attachmentsDS->addImage(
            static_cast<uint32_t>(binding),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _attachments->imageWithIndex(static_cast<uint32_t>(binding)).get(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _attSampler
        );
    }
    attachmentsDS->endUpdate();
    
    std::array<VkDescriptorSet, 1> descriptorSets = {
        attachmentsDS->descriptorSet()
    };
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout, 0,
        static_cast<uint32_t>(descriptorSets.size()),
        descriptorSets.data(),
        0, nullptr
    );
    vkCmdDraw(cmd, 6, 1, 0, 0);
    
    vulkan::cmdEndRendering(cmd);
}

}
