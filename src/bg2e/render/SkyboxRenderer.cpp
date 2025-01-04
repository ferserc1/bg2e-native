
#include <bg2e/render/SkyboxRenderer.hpp>

namespace bg2e::render {

SkyboxRenderer::SkyboxRenderer(Vulkan * vulkan)
    :CubemapRenderer { vulkan }
{

}
    
void SkyboxRenderer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
}

void SkyboxRenderer::build(
    std::shared_ptr<Texture> skyTexture,
    VkFormat colorAttachmentFormat,
    VkFormat depthAttachmentFormat,
    float cubeSize
) {
    CubemapRenderer::build(
        skyTexture,
        colorAttachmentFormat,
        depthAttachmentFormat,
        "skybox_renderer.vert.spv",
        "skybox_renderer.frag.spv",
        cubeSize
    );
}

}
