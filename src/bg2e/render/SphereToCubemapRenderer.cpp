
#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/db/image.hpp>

namespace bg2e::render {

SphereToCubemapRenderer::SphereToCubemapRenderer(Vulkan * vulkan)
    :_vulkan(vulkan)
{

}

void SphereToCubemapRenderer::initFrameResources(vulkan::FrameResources& frameResources)
{
    frameResources.descriptorAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
}

void SphereToCubemapRenderer::build(
    const std::filesystem::path& imagePath,
    const std::string& vertexShader,
    const std::string& fragmentShader,
    VkExtent2D cubeImageSize
) {
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsFactory.build(
        _vulkan->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    updateImage(imagePath);
    initImages(cubeImageSize);
    initPipeline(vertexShader, fragmentShader);
    initGeometry();
}

void SphereToCubemapRenderer::updateImage(const std::filesystem::path& imagePath)
{
    if (_skyTexture.get())
    {
        _skyTexture->cleanup();
    }
    
    auto image = bg2e::db::loadImage(imagePath);
    auto texture = new bg2e::base::Texture(image);
    texture->setMagFilter(bg2e::base::Texture::FilterLinear);
    texture->setMinFilter(bg2e::base::Texture::FilterLinear);

    _skyTexture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
        _vulkan,
        texture
    ));
}
    
void SphereToCubemapRenderer::update(VkCommandBuffer commandBuffer, vulkan::FrameResources& frameResources)
{

}

void SphereToCubemapRenderer::initImages(VkExtent2D)
{

}

void SphereToCubemapRenderer::initPipeline(const std::string& vshaderFile, const std::string& fshaderFile)
{

}

void SphereToCubemapRenderer::initGeometry()
{

}

}
