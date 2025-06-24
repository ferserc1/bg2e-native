//
//  GPUTextureGenerator.cpp
#include <bg2e/render/GPUTextureGenerator.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e::render {

GPUTextureGenerator::GPUTextureGenerator(Engine * engine, uint32_t w, uint32_t h)
    :_engine{engine}, _width{w}, _height{h}
{

}

vulkan::Image * GPUTextureGenerator::createImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage)
{
    return vulkan::Image::createAllocatedImage(
        _engine,
        format,
        extent,
        usage
    );
}

void GPUTextureGenerator::executeComputeShader(
    VkPipeline pipeline,
    VkPipelineLayout layout,
    const std::vector<VkDescriptorSet>& ds,
    vulkan::Image* image
) {
    _engine->command().immediateSubmit([&, pipeline, layout, ds, image](VkCommandBuffer cmd) {
        vulkan::Image::cmdTransitionImage(cmd, image->handle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            layout,
            0,
            static_cast<uint32_t>(ds.size()),
            ds.data(),
            0,
            nullptr
        );
        
        vkCmdDispatch(
            cmd,
            std::ceil(static_cast<uint32_t>(image->extent().width / 16.0f)),
            std::ceil(static_cast<uint32_t>(image->extent().height / 16.0f)),
            1
        );
        
        vulkan::Image::cmdTransitionImage(cmd, image->handle(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
}

Texture * GPUTextureGenerator::wrapImage(
    vulkan::Image * image,
    bool useMipmaps,
    base::Texture::Filter magFilter,
    base::Texture::Filter minFilter
) {
    auto baseTexture = new base::Texture();
    baseTexture->setUseMipmaps(useMipmaps);
    baseTexture->setMagFilter(magFilter);
    baseTexture->setMinFilter(minFilter);
    auto result = new Texture(_engine);
    result->load(baseTexture, image);
    return result;
}

}
