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
