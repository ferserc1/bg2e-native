//
//  SelectionManager.cpp

#include <bg2e/manipulation/SelectionManager.hpp>

#include <bg2e/render/all.hpp>

namespace bg2e::manipulation {

SelectionManager::SelectionManager(render::Engine * engine)
    : _engine(engine)
{

}

void SelectionManager::init()
{
    createImage();
    createPipeline();
    
    _pickVisitor = std::make_shared<manipulation::PickSelectionVisitor>();
}

bool SelectionManager::pick(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport &, // vp,
    uint32_t, // x
    uint32_t  // y
) {
    using namespace render::vulkan;
    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        VkClearColorValue clearValue { { 0.0f, 0.0f, 0.0f, 0.0f } };
        auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(
            cmd,
            _image->handle(), VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );
        
        Image::cmdTransitionImage(
            cmd,
            _image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
        
        auto colorAttachment = Info::attachmentInfo(_image->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(_engine->swapchain().depthImage()->imageView());
        auto renderInfo = Info::renderingInfo(_image->extent2D(), &colorAttachment, &depthAttachment);
        
        cmdBeginRendering(cmd, &renderInfo);
        
        macros::cmdSetDefaultViewportAndScissor(cmd, _image->extent2D());
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        _pickVisitor->pick(rootNode, viewMatrix, projMatrix, cmd, _pipelineLayout);
        
        cmdEndRendering(cmd);
        
        // TODO: Transition image (read pixels)
    });
    
    // TODO: Read pixels in image
    
    // TODO: Get buffer value at pick position (x, y)
    
    // TODO: Search drawable & submesh
    
    // TODO: Return true if item picked
    return false;
}

void SelectionManager::createImage()
{
    using namespace bg2e::render::vulkan;

    auto extent = _engine->swapchain().extent();
    _image = std::shared_ptr<render::vulkan::Image>(
        render::vulkan::Image::createAllocatedImage(
            _engine,
            VK_FORMAT_R8G8B8A8_UNORM,
            extent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    );
    
    _engine->cleanupManager().push([&](VkDevice) {
        _image.reset();
    });
}

void SelectionManager::createPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);
    
    plFactory.addShader("pick_selection.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("pick_selection.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &_pipelineLayout));
    
    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_image->format());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    _pipeline = plFactory.build(_pipelineLayout);
    
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
}

}
