//
//  GPUTextureGenerator.hpp
#pragma once

#include <bg2e/render/Texture.hpp>

namespace bg2e {
namespace render {

class BG2E_API GPUTextureGenerator {
public:
    GPUTextureGenerator(Engine * engine, uint32_t w, uint32_t h);
    virtual ~GPUTextureGenerator() = default;

    virtual Texture * generate() = 0;

    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    
protected:
    Engine * _engine;
    uint32_t _width;
    uint32_t _height;
    
    vulkan::Image * createImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage);
    Texture * wrapImage(
        vulkan::Image * image,
        bool useMipmaps,
        base::Texture::Filter magFilter,
        base::Texture::Filter minFilter
    );
};

}
}
