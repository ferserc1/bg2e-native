/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/draw/Texture.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/db/texture_archive.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bg2e::draw {
namespace {
// Snapshot is necessary because arbitrary procedural generators have mutable
// state and no clone API; cloned settings must never share that state.
class PixelSnapshot : public base::ProceduralTextureGenerator {
  public:
    explicit PixelSnapshot(base::Image& image)
        : ProceduralTextureGenerator(image.width(), image.height(), image.channels()),
          _pixels(image.data(), image.data() + size_t(image.width()) * image.height() * image.channels()),
          _colorType(image.colorType())
    {
    }
    uint8_t* generate() override
    {
        auto* data = new uint8_t[_pixels.size()];
        std::copy(_pixels.begin(), _pixels.end(), data);
        return data;
    }
    std::string imageIdentifier() override
    {
        return "draw-pixel-snapshot";
    }
    base::Color::Type colorType() override
    {
        return _colorType;
    }
  private:
    std::vector<uint8_t> _pixels;
    base::Color::Type _colorType;
};

std::unique_ptr<base::Image> copyPixels(const base::Image& image)
{
    const auto count = size_t(image.width()) * image.height() * image.channels();
    if (image.dataf())
    {
        auto data = std::make_unique<float[]>(count);
        std::copy_n(image.dataf(), count, data.get());
        return std::make_unique<base::Image>(data.release(), image.width(), image.height(), image.channels());
    }
    auto data = std::make_unique<uint8_t[]>(count);
    std::copy_n(image.data(), count, data.get());
    return std::make_unique<base::Image>(data.release(), image.width(), image.height(), image.channels());
}

gpu::AddressMode addressMode(base::Texture::AddressMode mode)
{
    switch (mode)
    {
    case base::Texture::AddressModeRepeat: return gpu::AddressMode::Repeat;
    case base::Texture::AddressModeMirroredRepeat: return gpu::AddressMode::MirroredRepeat;
    case base::Texture::AddressModeClampToEdge: return gpu::AddressMode::ClampToEdge;
    case base::Texture::AddressModeClampToBorder: return gpu::AddressMode::ClampToBorder;
    }
    throw std::invalid_argument("Unknown texture address mode");
}

gpu::SamplerDescription samplerDescription(const base::Texture& settings, uint32_t levels)
{
    gpu::SamplerDescription description;
    description.minFilter = settings.minFilter() == base::Texture::FilterLinear ? gpu::Filter::Linear : gpu::Filter::Nearest;
    description.magFilter = settings.magFilter() == base::Texture::FilterLinear ? gpu::Filter::Linear : gpu::Filter::Nearest;
    description.mipFilter = settings.minFilter() == base::Texture::FilterLinear ? gpu::MipmapFilter::Linear : gpu::MipmapFilter::Nearest;
    description.addressModeU = addressMode(settings.addressModeU());
    description.addressModeV = addressMode(settings.addressModeV());
    description.addressModeW = addressMode(settings.addressModeW());
    const float maxLevel = settings.useMipmaps() ? float(levels - 1) : 0.0f;
    description.maxLod = settings.maxLod() < 0 ? maxLevel : std::min(settings.maxLod(), maxLevel);
    description.minLod = std::min(settings.minLod(), description.maxLod);
    description.mipLodBias = settings.mipLodBias();
    return description;
}

bool sameSampler(const gpu::SamplerDescription& a, const gpu::SamplerDescription& b)
{
    return a.minFilter == b.minFilter && a.magFilter == b.magFilter && a.mipFilter == b.mipFilter &&
        a.addressModeU == b.addressModeU && a.addressModeV == b.addressModeV && a.addressModeW == b.addressModeW &&
        a.minLod == b.minLod && a.maxLod == b.maxLod && a.mipLodBias == b.mipLodBias;
}

void copySamplerSettings(base::Texture& target, const base::Texture& source)
{
    target.setMinFilter(source.minFilter());
    target.setMagFilter(source.magFilter());
    target.setUseMipmaps(source.useMipmaps());
    target.setMinLod(source.minLod());
    target.setMaxLod(source.maxLod());
    target.setMipLodBias(source.mipLodBias());
    target.setAddressMode(source.addressModeU(), source.addressModeV(), source.addressModeW());
}
}

std::shared_ptr<base::Texture> cloneTextureSettings(const base::Texture& source)
{
    auto settings = std::make_shared<base::Texture>(source);
    if (source.type() == base::Texture::TypeProcedural)
    {
        std::unique_ptr<base::Image> pixels(settings->generateImage());
        if (!pixels || !pixels->data() || pixels->width() == 0 || pixels->height() == 0 ||
            pixels->channels() == 0 || pixels->channels() > 4)
        {
            throw std::invalid_argument("Invalid procedural texture snapshot");
        }
        settings->setProceduralGenerator(std::make_shared<PixelSnapshot>(*pixels));
    }
    return settings;
}

Texture::Texture(Engine* engine) : _engine(engine), _settings(std::make_shared<base::Texture>())
{
    if (!engine)
    {
        throw std::invalid_argument("draw::Texture requires an engine");
    }
    engine->device();
}

Texture::Texture(Engine* engine, std::shared_ptr<base::Texture> settings) : Texture(engine)
{
    load(std::move(settings));
}

void Texture::load(std::shared_ptr<base::Texture> settings)
{
    if (!settings)
    {
        throw std::invalid_argument("draw::Texture requires settings");
    }
    auto independent = cloneTextureSettings(*settings);
    if (independent->type() == base::Texture::TypeFilesystem &&
        std::filesystem::path(independent->imageFilePath()).extension() == ".bg2tex")
    {
        auto archive = db::readTextureArchive(independent->imageFilePath());
        auto image = _engine->device()->createImage(archive.description);
        size_t index = 0;
        for (uint32_t layer = 0; layer < image->arrayLayers(); ++layer)
        {
            for (uint32_t mip = 0; mip < image->mipLevels(); ++mip)
            {
                const auto& bytes = archive.subresources.at(index++);
                image->upload({bytes.data(), bytes.size(), 0, mip, layer});
            }
        }
        load(independent, std::move(image));
        return;
    }
    std::unique_ptr<base::Image> pixels(independent->type() == base::Texture::TypeProcedural
        ? independent->generateImage() : db::loadImage(independent->imageFilePath()));
    load(*pixels, *independent);
}

void Texture::load(const base::Image& pixels, const base::Texture& settings)
{
    {
        base::CpuProfileScope profile("texture.waitIdle.load");
        base::CpuProfileScope::count("texture.drains.load", 1);
        _engine->device()->waitIdle();
    }
    gpu::ImageDescription description;
    description.size = {pixels.width(), pixels.height()};
    description.format = pixels.dataf() ? gpu::PixelFormat::R32G32B32A32_SFLOAT : gpu::PixelFormat::R8G8B8A8_UNORM;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    if (settings.useMipmaps())
    {
        description.mipLevels = gpu::mipLevelsForMinSize(std::max(description.size.width, description.size.height), 1);
        if (description.mipLevels > 1 && !_engine->device()->formatCapabilities(description.format).mipmapGeneration)
        {
            throw std::runtime_error("Texture format does not support requested mipmap generation");
        }
    }
    auto image = _engine->device()->createImage(description);
    image->uploadImage(&pixels);
    if (description.mipLevels > 1)
    {
        image->generateMipmaps();
    }
    auto memorySource = settings.type() == base::Texture::TypeFilesystem && settings.imageFilePath().empty()
        ? copyPixels(pixels) : nullptr;
    load(cloneTextureSettings(settings), std::move(image));
    _memorySource = std::move(memorySource);
}

void Texture::load(std::shared_ptr<base::Texture> settings, std::shared_ptr<gpu::Image> image)
{
    if (!settings || !image || !image->isValid() || image->device() != _engine->device() ||
        gpu::isDepthFormat(image->pixelFormat()) ||
        (image->usage() & gpu::ImageUsage::Sampled) == gpu::ImageUsage::None)
    {
        throw std::invalid_argument("draw::Texture image belongs to an incompatible context or usage");
    }
    if ((settings->minFilter() == base::Texture::FilterLinear || settings->magFilter() == base::Texture::FilterLinear) &&
        !_engine->device()->formatCapabilities(image->pixelFormat()).linearFiltering)
    {
        throw std::invalid_argument("Requested texture filtering is unsupported for this format");
    }
    auto independent = cloneTextureSettings(*settings);
    auto description = samplerDescription(*independent, image->mipLevels());
    auto sampler = _engine->device()->createSampler(description);
    {
        base::CpuProfileScope profile("texture.waitIdle.load");
        base::CpuProfileScope::count("texture.drains.load", 1);
        _engine->device()->waitIdle();
    }
    _memorySource.reset();
    _settings = std::move(independent);
    _image = std::move(image);
    _sampler = std::move(sampler);
    _samplerDescription = description;
    ++_bindingRevision;
}

void Texture::reload()
{
    if (_memorySource)
    {
        load(*_memorySource, *_settings);
    }
    else
    {
        load(_settings);
    }
}

void Texture::updateSampler(const base::Texture& settings)
{
    if (!_image)
    {
        throw std::logic_error("Texture is not loaded");
    }
    if ((settings.minFilter() == base::Texture::FilterLinear || settings.magFilter() == base::Texture::FilterLinear) &&
        !_engine->device()->formatCapabilities(_image->pixelFormat()).linearFiltering)
    {
        throw std::invalid_argument("Requested texture filtering is unsupported for this format");
    }
    const auto description = samplerDescription(settings, _image->mipLevels());
    if (_sampler && _sampler->isValid() && sameSampler(description, _samplerDescription))
    {
        // Preserve authored settings even if clamping leaves native state equal.
        copySamplerSettings(*_settings, settings);
        return;
    }
    auto independent = cloneTextureSettings(*_settings);
    copySamplerSettings(*independent, settings);
    auto sampler = _engine->device()->createSampler(description);
    {
        base::CpuProfileScope profile("texture.waitIdle.updateSampler");
        base::CpuProfileScope::count("texture.drains.updateSampler", 1);
        _engine->device()->waitIdle();
    }
    _settings = std::move(independent);
    _sampler = std::move(sampler);
    _samplerDescription = description;
    ++_bindingRevision;
}

std::shared_ptr<Texture> Texture::clone() const
{
    if (!_image || !_image->isValid())
    {
        throw std::logic_error("Cannot clone an unloaded texture");
    }
    {
        base::CpuProfileScope profile("texture.waitIdle.clone");
        base::CpuProfileScope::count("texture.drains.clone", 1);
        _engine->device()->waitIdle();
    }
    if ((_image->usage() & gpu::ImageUsage::TransferSrc) == gpu::ImageUsage::None)
    {
        throw std::invalid_argument("Texture cloning requires transfer-source usage");
    }
    gpu::ImageDescription description;
    description.size = _image->size();
    description.format = _image->pixelFormat();
    description.type = _image->imageType();
    description.usage = _image->usage() | gpu::ImageUsage::TransferDst | gpu::ImageUsage::TransferSrc;
    description.mipLevels = _image->mipLevels();
    for (uint32_t layer = 0; layer < _image->arrayLayers(); ++layer)
    {
        for (uint32_t mip = 0; mip < _image->mipLevels(); ++mip)
        {
            if (_image->currentLayout({mip, 1, layer, 1}) == gpu::ImageLayout::Undefined)
            {
                throw std::invalid_argument("Cannot clone uninitialized texture subresources");
            }
        }
    }
    auto image = _engine->device()->createImage(description);
    _engine->device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        for (uint32_t layer = 0; layer < _image->arrayLayers(); ++layer)
        {
            for (uint32_t mip = 0; mip < _image->mipLevels(); ++mip)
            {
                const gpu::ImageSubresourceRange range{mip, 1, layer, 1};
                const auto previous = _image->currentLayout(range);
                command->transition(_image.get(), gpu::ImageLayout::TransferSrc, range);
                command->transition(image.get(), gpu::ImageLayout::TransferDst, range);
                command->copyImage(_image->view(range), image->view(range));
                command->transition(_image.get(), previous, range);
                command->transition(image.get(), gpu::ImageLayout::ShaderReadOnly, range);
            }
        }
    });
    auto result = std::make_shared<Texture>(_engine);
    result->load(cloneTextureSettings(*_settings), std::move(image));
    if (_memorySource)
    {
        result->_memorySource = copyPixels(*_memorySource);
    }
    return result;
}

std::shared_ptr<Texture> Texture::colorTexture(Engine* engine, const base::Color& color)
{
    auto result = std::make_shared<Texture>(engine);
    uint8_t* data = new uint8_t[4];
    for (uint32_t i = 0; i < 4; ++i)
    {
        const float value = i == 0 ? color.r : i == 1 ? color.g : i == 2 ? color.b : color.a;
        data[i] = uint8_t(std::lround(std::clamp(value, 0.0f, 1.0f) * 255));
    }
    base::Image pixels(data, 1, 1, 4);
    base::Texture settings;
    settings.setUseMipmaps(false);
    settings.setColorType(base::Color::TypeLinear);
    result->load(pixels, settings);
    return result;
}
}
