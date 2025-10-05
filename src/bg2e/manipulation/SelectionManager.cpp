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
}

bool SelectionManager::pick(scene::Node * rootNode, const math::Viewport & vp, uint32_t x, uint32_t y)
{
    return false;
}

void SelectionManager::createImage()
{
    using namespace bg2e::render::vulkan;

    // TODO: Create image
}

void SelectionManager::createPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);
    
    plFactory.addShader("pick_selection.vert.glsl", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("pick_selection.frag.glsl", VK_SHADER_STAGE_FRAGMENT_BIT);
    
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
