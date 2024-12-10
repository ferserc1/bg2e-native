
#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
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

void SphereToCubemapRenderer::cleanup()
{
    vkDestroyPipeline(_vulkan->device().handle(), _pipeline, nullptr);
    vkDestroyPipelineLayout(_vulkan->device().handle(), _pipelineLayout, nullptr);
    
    for (int32_t i = 0; i < 6; ++i)
    {
        vkDestroyImageView(_vulkan->device().handle(), _cubeMapImageViews[i], nullptr);
    }
    _cubeMapImage->cleanup();
    _skyTexture->cleanup();
    vkDestroyDescriptorSetLayout(_vulkan->device().handle(), _dsLayout, nullptr);
}

void SphereToCubemapRenderer::initImages(VkExtent2D extent)
{
    _cubeMapImage = std::shared_ptr<bg2e::render::vulkan::Image>(bg2e::render::vulkan::Image::createAllocatedImage(
        _vulkan,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        6
    ));
    
    auto viewInfo = bg2e::render::vulkan::Info::imageViewCreateInfo(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        _cubeMapImage->handle(),
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    
    VkImageView imgView;
    for (int32_t i = 0; i < 6; ++i)
    {
        viewInfo.subresourceRange.baseArrayLayer = i;
        vkCreateImageView(_vulkan->device().handle(), &viewInfo, nullptr, &imgView);
        _cubeMapImageViews[i] = imgView;
    }
}

void SphereToCubemapRenderer::initPipeline(const std::string& vshaderFile, const std::string& fshaderFile)
{
    vulkan::factory::GraphicsPipeline plFactory(_vulkan);
    
    plFactory.addShader(vshaderFile, VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader(fshaderFile, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    plFactory.setInputState<vulkan::geo::MeshPU>();
    
    vulkan::factory::PipelineLayout layoutFactory(_vulkan);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(RenderSpherePushConstant),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    _pipelineLayout = layoutFactory.build();
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.disableDepthtest();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    
    _pipeline = plFactory.build(_pipelineLayout);
}

void SphereToCubemapRenderer::initGeometry()
{

}

}
