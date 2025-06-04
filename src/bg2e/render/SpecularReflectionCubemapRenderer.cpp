
#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e::render {

SpecularReflectionCubemapRenderer::SpecularReflectionCubemapRenderer(Engine * engine)
    :CubemapRenderer(engine)
{
}
    
void SpecularReflectionCubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    });
}

void SpecularReflectionCubemapRenderer::build(
    std::shared_ptr<vulkan::Image> inputCubemap,
    VkExtent2D cubeImageSize
) {
    CubemapRenderer::build(
        inputCubemap,
        "cubemap_renderer.vert.spv",
        "specular_reflection_renderer.frag.spv",
        cubeImageSize,
        true,
        10,
        VK_FORMAT_B8G8R8A8_UNORM
    );
}

void SpecularReflectionCubemapRenderer::createPipelineLayout()
{
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // Projection data buffer
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Input skybox image
    _descriptorSetLayout = dsFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vulkan::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    std::vector<VkDescriptorSetLayout> layouts = {
        _descriptorSetLayout
    };
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.setLayoutCount = uint32_t(layouts.size());
    
    VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &_layout));
    
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipelineLayout(dev, _layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _descriptorSetLayout, nullptr);
    });
}

}
