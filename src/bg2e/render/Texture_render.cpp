
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/Engine.hpp>
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


std::shared_ptr<Texture> Texture::_blackTexture;
std::shared_ptr<Texture> Texture::_whiteTexture;
std::shared_ptr<Texture> Texture::_normalTexture;

Texture * Texture::colorTexture(Engine *engine, const base::Color &color, VkExtent2D size)
{
    uint32_t bytes = 4;
    size_t dataSize = size.width * size.height * bytes;
    std::vector<uint8_t> data(dataSize, 0);
    
    for (auto i = 0; i < dataSize; i += bytes)
    {
        data[i] = static_cast<uint8_t>(color.r * 255.0f);
        data[i + 1] = static_cast<uint8_t>(color.g * 255.0f);
        data[i + 2] = static_cast<uint8_t>(color.b * 255.0f);
        data[i + 3] = static_cast<uint8_t>(color.a * 255.0f);
    }
    
    auto image = vulkan::Image::createAllocatedImage(
        engine,
        reinterpret_cast<void*>(data.data()),
        size, bytes,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false
    );
    
    return new Texture(engine, image);
}

std::shared_ptr<Texture> Texture::blackTexture(Engine *engine)
{
    if (_blackTexture.get() == nullptr)
    {
        _blackTexture = std::shared_ptr<Texture>(colorTexture(engine, base::Color::Black(), { 2, 2 }));
        engine->cleanupManager().push([](VkDevice dev) {
            _blackTexture.reset();
        });
    }
    return _blackTexture;
}

std::shared_ptr<Texture> Texture::whiteTexture(Engine *engine)
{
    if (_whiteTexture.get() == nullptr)
    {
        _whiteTexture = std::shared_ptr<Texture>(colorTexture(engine, base::Color::White(), { 2, 2 }));
        engine->cleanupManager().push([](VkDevice dev) {
            _whiteTexture.reset();
        });
    }
    return _whiteTexture;
}

std::shared_ptr<Texture> Texture::normalTexture(Engine *engine)
{
    if (_normalTexture.get() == nullptr)
    {
        _normalTexture = std::shared_ptr<Texture>(colorTexture(engine, base::Color(0.5f, 0.5f, 1.0f, 1.0f), { 2, 2 }));
        engine->cleanupManager().push([](VkDevice dev) {
            _normalTexture.reset();
        });
    }
    return _normalTexture;
}

Texture::~Texture()
{
    cleanup();
}

void Texture::load(std::shared_ptr<base::Texture> texture)
{
    _texture = texture;
    
    std::unique_ptr<base::Image> img;
    
    switch (_texture->type())
    {
        case base::Texture::TypeFilesystem:
            img = std::unique_ptr<base::Image>(db::loadImage(texture->imageFilePath()));
            break;
        case base::Texture::TypeProcedural:
            img = std::unique_ptr<base::Image>(texture->generateImage());
            break;
    }

    
    // TODO: Extract image options, such as mipmap levels
    // TODO: Support for HDR images
    VkExtent2D extent = { img->width(), img->height() };
    _hasImageOwnership = true;
    _image = std::shared_ptr<vulkan::Image>(vulkan::Image::createAllocatedImage(
        _engine,
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
        _engine->device().handle(),
        &samplerInfo,
        nullptr,
        &_sampler
    );
    
    _colorType = texture->colorType();
}

void Texture::load(std::shared_ptr<base::Texture> texture, std::shared_ptr<vulkan::Image> image)
{
    _texture = texture;
    _image = image;
    _hasImageOwnership = false;
    _colorType = texture->colorType();
    
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
        _engine->device().handle(),
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
    vkDestroySampler(_engine->device().handle(), _sampler, nullptr);
}


}
}
