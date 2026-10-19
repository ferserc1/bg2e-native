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
#include <algorithm>
#include <numbers>
#include <stdexcept>
#include <utility>

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
        if (!pixels || !pixels->dataf() || pixels->width() == 0 || pixels->height() == 0)
        {
            throw std::runtime_error("Environment file must contain linear floating-point HDR pixels: " + sourcePath.string());
        }
        auto* device = _engine->device();
        double maximum = 0;
        for (size_t i = 0; i < size_t(pixels->width()) * pixels->height(); ++i)
        {
            for (uint32_t channel = 0; channel < std::min(3u, pixels->channels()); ++channel)
            {
                const auto value = pixels->dataf()[i * pixels->channels() + channel];
                if (!std::isfinite(value))
                {
                    throw std::runtime_error("Environment HDR pixels must be finite: " + sourcePath.string());
                }
                maximum = std::max(maximum, double(std::abs(value)));
            }
        }
        // Irradiance integrates radiance over a cosine-weighted hemisphere.
        // Leave rounding headroom below half's maximum, including the PI gain.
        const auto format = maximum * std::numbers::pi > 65000.0
            ? gpu::PixelFormat::R32G32B32A32_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT;
        const auto capabilities = device->formatCapabilities(format);
        const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
        if (!capabilities.supports(usage))
        {
            throw std::runtime_error("Device cannot upload the HDR environment format");
        }
        source = device->createImage({.size = {pixels->width(), pixels->height()}, .format = format, .usage = usage,
            .minMipSize = capabilities.mipmapGeneration ? 1u : 0u, .debugName = "Linear HDR equirectangular source"});
        source->uploadImage(pixels.get());
        source->generateMipmaps();
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
