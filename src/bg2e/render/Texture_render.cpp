
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/db/image.hpp>

#define BG2E_ADDRESS_MODE(x)        x == base::Texture::AddressModeRepeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : \
                                    x == base::Texture::AddressModeMirroredRepeat ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT : \
                                    x == base::Texture::AddressModeClampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : \
                                    x == base::Texture::AddressModeClampToBorder ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : \
                                    VK_SAMPLER_ADDRESS_MODE_REPEAT

#define BG2E_FILTER(x)              x == base::Texture::FilterLinear ? VK_FILTER_LINEAR : \
                                    x == base::Texture::FilterNearest ? VK_FILTER_NEAREST: \
                                    VK_FILTER_LINEAR

namespace bg2e {
namespace render {

Texture::~Texture()
{
    cleanup();
}

void Texture::load(std::shared_ptr<base::Texture> texture)
{
    _texture = texture;
    
    std::unique_ptr<base::Image> img = std::unique_ptr<base::Image>(db::loadImage(texture->imageFilePath()));
    
    // TODO: Extract image options, such as mipmap levels
    // TODO: Support for HDR images
    VkExtent2D extent = { img->width(), img->height() };
    _hasImageOwnership = true;
    _image = std::shared_ptr<vulkan::Image>(vulkan::Image::createAllocatedImage(
        _vulkan,
        img->data(),
        extent,
        4,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        texture->useMipmaps()
    ));
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = BG2E_FILTER(texture->magFilter());
    samplerInfo.minFilter = BG2E_FILTER(texture->minFilter());
    samplerInfo.maxLod = texture->maxLod() == -1.0f ? float(_image->mipLevels()) : texture->maxLod();
    samplerInfo.minLod = texture->minLod();
    samplerInfo.addressModeU = BG2E_ADDRESS_MODE(texture->addressModeU());
    samplerInfo.addressModeV = BG2E_ADDRESS_MODE(texture->addressModeV());
    samplerInfo.addressModeW = BG2E_ADDRESS_MODE(texture->addressModeW());
    vkCreateSampler(
        _vulkan->device().handle(),
        &samplerInfo,
        nullptr,
        &_sampler
    );
    
}

void Texture::load(std::shared_ptr<base::Texture> texture, std::shared_ptr<vulkan::Image> image)
{
    _texture = texture;
    _image = image;
    _hasImageOwnership = false;
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = BG2E_FILTER(texture->magFilter());
    samplerInfo.minFilter = BG2E_FILTER(texture->minFilter());
    samplerInfo.maxLod = texture->maxLod() == -1.0f ? float(_image->mipLevels()) : texture->maxLod();
    samplerInfo.minLod = texture->minLod();
    samplerInfo.addressModeU = BG2E_ADDRESS_MODE(texture->addressModeU());
    samplerInfo.addressModeV = BG2E_ADDRESS_MODE(texture->addressModeV());
    samplerInfo.addressModeW = BG2E_ADDRESS_MODE(texture->addressModeW());
    vkCreateSampler(
        _vulkan->device().handle(),
        &samplerInfo,
        nullptr,
        &_sampler
    );
}

void Texture::cleanup()
{
    if (_hasImageOwnership)
    {
        _image->cleanup();
    }
    _image.reset();
    _texture.reset();
    vkDestroySampler(_vulkan->device().handle(), _sampler, nullptr);
}


}
}
