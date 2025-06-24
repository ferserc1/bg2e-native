
#include <bg2e/render/IrradianceCubemapRenderer.hpp>

namespace bg2e::render {

IrradianceCubemapRenderer::IrradianceCubemapRenderer(Engine * engine)
    :CubemapRenderer(engine)
{
}
    
void IrradianceCubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* frameAllocator)
{
    CubemapRenderer::initFrameResources(frameAllocator);
}

void IrradianceCubemapRenderer::build(
    std::shared_ptr<vulkan::Image> inputCubemap,
    VkExtent2D cubeImageSize
) {
    CubemapRenderer::build(
        inputCubemap,
        "cubemap_renderer.vert.spv",
        "irradiance_map_renderer.frag.spv",
        { 16, 16 },
        false,
        1,
        VK_FORMAT_R32G32B32A32_SFLOAT
    );
}

}
