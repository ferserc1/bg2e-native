
#include <bg2e/render/IrradianceCubemapRenderer.hpp>

namespace bg2e::render {

IrradianceCubemapRenderer::IrradianceCubemapRenderer(Vulkan * vulkan)
    :CubemapRenderer(vulkan)
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
        VK_FORMAT_B8G8R8A8_UNORM
    );
}

}
