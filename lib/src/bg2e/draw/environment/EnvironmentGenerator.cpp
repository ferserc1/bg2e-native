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

#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/draw/environment/SphereToCubemapRenderer.hpp>
#include <bg2e/draw/environment/IrradianceCubemapRenderer.hpp>
#include <bg2e/draw/environment/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/draw/environment/BRDFIntegrationMapTextureGenerator.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Sampler.hpp>

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bg2e::draw::environment {
void EnvironmentResources::validate(gpu::Device* device) const
{
    if (!device || !sampler || !sampler->isValid() || sampler->device() != device ||
        !brdfSampler || !brdfSampler->isValid() || brdfSampler->device() != device ||
        !std::isfinite(skyboxBlur) || skyboxBlur < 0 || skyboxBlur > 1)
    {
        throw std::invalid_argument("Environment samplers/settings belong to an invalid context");
    }
    for (const auto& image : {sky, irradiance, prefilteredSpecular, brdf})
    {
        if (!image || !image->isValid() || image->device() != device ||
            !gpu::hasFlag(image->usage(), gpu::ImageUsage::Sampled) ||
            (image->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT && image->pixelFormat() != gpu::PixelFormat::R32G32B32A32_SFLOAT) ||
            image->currentLayout(image->fullRange()) != gpu::ImageLayout::ShaderReadOnly)
        {
            throw std::invalid_argument("Environment requires complete same-device shader-readable linear HDR images");
        }
    }
    if (!sky->isCubemap() || !irradiance->isCubemap() || !prefilteredSpecular->isCubemap() || brdf->isCubemap() ||
        irradiance->mipLevels() != 1 || brdf->mipLevels() != 1 || prefilteredSpecular->mipLevels() < 2)
    {
        throw std::invalid_argument("Invalid environment image type/mip layout");
    }
}
EnvironmentGenerator::EnvironmentGenerator(Engine& engine, const std::filesystem::path& shaderRoot)
    : _engine(&engine), _shaderRoot(shaderRoot)
{
    _configuration.sphere = std::make_shared<SphereToCubemapRenderer>();
    _configuration.defaultSky = std::make_shared<ProceduralSkyRenderer>();
    _configuration.irradiance = std::make_shared<IrradianceCubemapRenderer>();
    _configuration.specular = std::make_shared<SpecularReflectionCubemapRenderer>();
    _configuration.brdf = std::make_shared<BRDFIntegrationMapTextureGenerator>();
}
std::shared_ptr<const EnvironmentResources> EnvironmentGenerator::build(const std::filesystem::path& hdrSource)
{
    const auto sourcePath = hdrSource.empty() ? hdrSource : std::filesystem::absolute(hdrSource).lexically_normal();
    std::shared_ptr<gpu::Image> source;
    if (!sourcePath.empty())
    {
        std::unique_ptr<base::Image> pixels(db::loadImage(sourcePath));
        if (!pixels || (!pixels->dataf() && !pixels->data()) || pixels->width() == 0 ||
            pixels->height() == 0 || pixels->channels() < 3)
        {
            throw std::runtime_error("Environment file must contain a readable RGB image: " + sourcePath.string());
        }
        auto* device = _engine->device();
        const size_t pixelCount = size_t(pixels->width()) * pixels->height();
        const bool srgbSource = !pixels->dataf() && pixels->colorType() == base::Color::TypeSRGB;
        const bool isHdr = pixels->dataf() != nullptr;
        std::vector<float> sourcePixels(pixelCount * 4, 0.0f);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            for (uint32_t channel = 0; channel < 3; ++channel)
            {
                float encoded = 0.0f;
                if (pixels->dataf())
                {
                    encoded = pixels->dataf()[i * pixels->channels() + channel];
                }
                else
                {
                    encoded = float(pixels->data()[i * pixels->channels() + channel]) / 255.0f;
                }
                if (!std::isfinite(encoded))
                {
                    throw std::runtime_error("Environment pixels must be finite: " + sourcePath.string());
                }
                sourcePixels[i * 4 + channel] = encoded;
            }
            sourcePixels[i * 4 + 3] = 1.0f;
        }
        // Keep HDR precision when required. Normalized SDR inputs safely fit in half.
        const auto format = isHdr
            ? gpu::PixelFormat::R32G32B32A32_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT;
        const auto capabilities = device->formatCapabilities(format);
        auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
        if (capabilities.mipmapGeneration)
        {
            usage |= gpu::ImageUsage::Storage;
        }
        if (!capabilities.supports(usage))
        {
            throw std::runtime_error("Device cannot upload the linear environment format");
        }
        if (srgbSource)
        {
            _configuration.sphere->setSourceSrgb(true);
            source = device->createImage({.size = {pixels->width(), pixels->height()}, .format = format, .usage = usage,
                .minMipSize = 0u,
                .debugName = "Linear equirectangular environment source"});
            source->upload({sourcePixels.data(), sourcePixels.size() * sizeof(float)});
        }
        else
        {
            _configuration.sphere->setSourceSrgb(false);
            source = device->createImage({.size = {pixels->width(), pixels->height()}, .format = format, .usage = usage,
                .minMipSize = 0u,
                .debugName = "Linear equirectangular environment source"});
            source->upload({sourcePixels.data(), sourcePixels.size() * sizeof(float)});
        }
        // The sphere conversion shader filters encoded SDR samples before
        // decoding them, so mip generation would incorrectly average sRGB.
        // Keep one source level for both color spaces; the cube generator uses
        // its own mip range and sampling budget.
    }
    auto result = generate(std::move(source));
    _rebuildRequested = false;
    _sourcePath = sourcePath;
    _pathSource = true;
    return result;
}
std::shared_ptr<const EnvironmentResources> EnvironmentGenerator::build(std::shared_ptr<gpu::Image> linearEquirectangular)
{
    if (!linearEquirectangular)
    {
        throw std::invalid_argument("Explicit environment image cannot be null; use an empty path for default sky");
    }
    auto result = generate(std::move(linearEquirectangular));
    _rebuildRequested = false;
    _pathSource = false;
    return result;
}
std::shared_ptr<const EnvironmentResources> EnvironmentGenerator::generate(std::shared_ptr<gpu::Image> source)
{
    auto conversion = source ? _configuration.sphere : _configuration.defaultSky;
    for (const auto& tool : {conversion, _configuration.irradiance, _configuration.specular, _configuration.brdf})
    {
        if (!tool)
        {
            throw std::invalid_argument("Environment generation tool selection cannot be null");
        }
        if (tool->settings().shaderRoot.empty())
        {
            tool->settings().shaderRoot = _shaderRoot;
        }
    }
    if (conversion->contract().shaderInterface != "draw.environment.sky.v1" ||
        _configuration.irradiance->contract().shaderInterface != "draw.environment.irradiance.v1" ||
        _configuration.specular->contract().shaderInterface != "draw.environment.prefilteredSpecular.v1" ||
        _configuration.brdf->contract().shaderInterface != "draw.environment.brdf.v1")
    {
        throw std::invalid_argument("Environment replacement tool has an incompatible semantic contract");
    }
    _engine->device()->waitIdle();
    auto next = std::make_shared<EnvironmentResources>();
    // A wide-range source must not overflow again in an intermediate cubemap.
    // Preserve requested settings; only the realized generation uses promotion.
    const bool wideRadiance = source && source->pixelFormat() == gpu::PixelFormat::R32G32B32A32_SFLOAT;
    const auto radiance = [&](const std::shared_ptr<CubemapRenderer>& tool, std::shared_ptr<gpu::Image> input)
    {
        const auto requested = tool->settings().format;
        if (wideRadiance)
        {
            tool->settings().format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
        }
        try
        {
            auto output = tool->generate(*_engine, std::move(input));
            tool->settings().format = requested;
            return output;
        }
        catch (...)
        {
            tool->settings().format = requested;
            throw;
        }
    };
    next->sky = radiance(conversion, std::move(source));
    next->irradiance = radiance(_configuration.irradiance, next->sky);
    next->prefilteredSpecular = radiance(_configuration.specular, next->sky);
    next->brdf = _configuration.brdf->generate(*_engine);
    bool linear = true;
    for (const auto& image : {next->sky, next->irradiance, next->prefilteredSpecular})
    {
        linear = linear && image && _engine->device()->formatCapabilities(image->pixelFormat()).linearFiltering;
    }
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = linear ? gpu::Filter::Linear : gpu::Filter::Nearest;
    sampler.mipFilter = linear ? gpu::MipmapFilter::Linear : gpu::MipmapFilter::Nearest;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = gpu::AddressMode::ClampToEdge;
    sampler.maxLod = float(next->prefilteredSpecular ? next->prefilteredSpecular->mipLevels() - 1 : 0);
    next->sampler = _engine->device()->createSampler(sampler);
    const bool brdfLinear = next->brdf && _engine->device()->formatCapabilities(next->brdf->pixelFormat()).linearFiltering;
    sampler.minFilter = sampler.magFilter = brdfLinear ? gpu::Filter::Linear : gpu::Filter::Nearest;
    sampler.maxLod = 0;
    sampler.mipFilter = gpu::MipmapFilter::Nearest;
    next->brdfSampler = _engine->device()->createSampler(sampler);
    next->validate(_engine->device());
    _resources = next;
    return next;
}
void EnvironmentGenerator::requestRebuild()
{
    _rebuildRequested = true;
}

std::shared_ptr<const EnvironmentResources> EnvironmentGenerator::buildFallback()
{
    auto* device = _engine->device();
    device->waitIdle();
    auto next = std::make_shared<EnvironmentResources>();
    const auto image = [&](gpu::ImageType type, uint32_t size, uint32_t mips)
    {
        gpu::ImageDescription description;
        description.size = {size, size};
        description.type = type;
        description.mipLevels = mips;
        description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        description.debugName = "Fallback black environment";
        auto result = device->createImage(description);
        for (uint32_t mip = 0; mip < mips; ++mip)
        {
            const auto extent = result->mipExtent(mip);
            std::vector<uint16_t> pixels(size_t(extent.width) * extent.height * 4, 0);
            for (size_t i = 3; i < pixels.size(); i += 4)
            {
                pixels[i] = 0x3c00; // IEEE half 1.0 alpha.
            }
            for (uint32_t face = 0; face < result->arrayLayers(); ++face)
            {
                result->upload({pixels.data(), pixels.size() * sizeof(uint16_t), 0, mip, face});
            }
        }
        return result;
    };
    next->sky = image(gpu::ImageType::Cubemap, 1, 1);
    next->irradiance = next->sky;
    next->prefilteredSpecular = image(gpu::ImageType::Cubemap, 2, 2);
    next->brdf = image(gpu::ImageType::Image2D, 1, 1);
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = gpu::AddressMode::ClampToEdge;
    sampler.maxLod = 1;
    next->sampler = device->createSampler(sampler);
    sampler.maxLod = 0;
    next->brdfSampler = device->createSampler(sampler);
    next->validate(device);
    _resources = next;
    _rebuildRequested = false;
    _sourcePath.clear();
    _pathSource = true;
    return next;
}

void EnvironmentGenerator::resetSourceCache()
{
    _sourcePath.clear();
    _pathSource = false;
    _rebuildRequested = true;
}

bool EnvironmentGenerator::synchronize(const scene::EnvironmentComponent* component)
{
    const std::filesystem::path path = component && !component->environmentImage().empty()
        ? std::filesystem::absolute(component->environmentImage()).lexically_normal() : std::filesystem::path{};
    if (_configuration.defaultSky)
    {
        auto& settings = _configuration.defaultSky->settings();
        const auto skyHorizonColor = component ? component->skyHorizonColor() : base::Color{0.55f, 0.65f, 0.8f};
        if (settings.skyHorizonColor.r != skyHorizonColor.r || settings.skyHorizonColor.g != skyHorizonColor.g || settings.skyHorizonColor.b != skyHorizonColor.b)
        {
            settings.skyHorizonColor = skyHorizonColor;
            if (path.empty())
            {
                _rebuildRequested = true;
            }
        }
        const auto skyZenithColor = component ? component->skyZenithColor() : base::Color{0.06f, 0.22f, 0.6f};
        if (settings.skyZenithColor.r != skyZenithColor.r || settings.skyZenithColor.g != skyZenithColor.g || settings.skyZenithColor.b != skyZenithColor.b)
        {
            settings.skyZenithColor = skyZenithColor;
            if (path.empty())
            {
                _rebuildRequested = true;
            }
        }
        const auto groundColor = component ? component->groundColor() : base::Color{0.04f, 0.035f, 0.025f};
        if (settings.groundColor.r != groundColor.r || settings.groundColor.g != groundColor.g || settings.groundColor.b != groundColor.b)
        {
            settings.groundColor = groundColor;
            if (path.empty())
            {
                _rebuildRequested = true;
            }
        }
    }
    bool changed = _rebuildRequested || !_resources || !_pathSource || _sourcePath != path;
    if (changed)
    {
        build(path);
    }
    const float blur = component ? component->skyboxBlur() : 0.0f;
    const bool visible = !component || component->skyboxVisible();
    if (_resources->skyboxBlur != blur || _resources->skyboxVisible != visible)
    {
        auto next = std::make_shared<EnvironmentResources>(*_resources);
        next->skyboxBlur = blur;
        next->skyboxVisible = visible;
        _resources = std::move(next);
        changed = true;
    }
    return changed;
}
void EnvironmentGenerator::cleanup()
{
    _rebuildRequested = false;
    _engine->device()->waitIdle();
    for (const auto& tool : {_configuration.sphere, _configuration.defaultSky, _configuration.irradiance, _configuration.specular, _configuration.brdf})
    {
        if (tool)
        {
            tool->cleanup();
        }
    }
    _resources.reset();
    _sourcePath.clear();
    _pathSource = false;
}
}
