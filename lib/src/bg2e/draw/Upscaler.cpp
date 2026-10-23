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

#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace bg2e::draw {
void UpscalerOpaqueMotion::validate(const gpu::Device& device, gpu::Size2D internalExtent) const
{
    if (bool(motion) != bool(token) || (motionOwner && motionOwner.get() != motion) ||
        (tokenOwner && tokenOwner.get() != token))
    {
        throw std::invalid_argument("Opaque motion requires a token/motion pair and matching optional retained owners");
    }
    if (!motion)
    {
        return;
    }
    for (auto* image : {motion,token})
    {
        if (!image->isValid() || image->device() != &device || image->imageType() != gpu::ImageType::Image2D ||
            image->size() != internalExtent || internalExtent.isZero() || image->arrayLayers() != 1 || image->mipLevels() != 1 ||
            !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
        {
            throw std::invalid_argument("Opaque motion images require matching same-device sampled single-level 2D receiver images");
        }
    }
    if (motion->pixelFormat() != gpu::PixelFormat::R16G16_SFLOAT || token->pixelFormat() != gpu::PixelFormat::R32_UINT)
    {
        throw std::invalid_argument("Opaque motion requires RG16F motion and R32_UINT tokens");
    }
}


float ResolutionSettings::scale() const
{
    return _scale;
}
void ResolutionSettings::setScale(float value)
{
    if (!std::isfinite(value) || value < 0.25f || value > 1.0f)
    {
        throw std::invalid_argument("Resolution scale must be between 0.25 and 1");
    }
    _scale = value;
}
bool ResolutionSettings::enabled() const
{
    return _enabled;
}
void ResolutionSettings::setEnabled(bool value)
{
    _enabled = value;
}
gpu::Size2D ResolutionSettings::resolve(gpu::Size2D output, gpu::Size2D fixed) const
{
    if (!_enabled)
    {
        return fixed;
    }
    // Integer arithmetic keeps preset percentages exact at pixel boundaries.
    // float(0.35) would otherwise round an exact 35% boundary down again.
    uint32_t percent = 0;
    if (_scale == 1.0f)
    {
        percent = 100;
    }
    else if (_scale == 0.60f)
    {
        percent = 60;
    }
    else if (_scale == 0.55f)
    {
        percent = 55;
    }
    else if (_scale == 0.50f)
    {
        percent = 50;
    }
    else if (_scale == 0.35f)
    {
        percent = 35;
    }
    if (percent)
    {
        return {std::max(1u,uint32_t(uint64_t(output.width) * percent / 100)),
            std::max(1u,uint32_t(uint64_t(output.height) * percent / 100))};
    }
    return {std::max(1u, uint32_t(std::floor(double(output.width) * _scale))),
        std::max(1u, uint32_t(std::floor(double(output.height) * _scale)))};
}
reflection::TypeInfo ResolutionSettings::settingsType() const
{
    reflection::TypeInfoBuilder<ResolutionSettings> type("bg2e.draw.resolution.v1");
    type.property("enabled", &ResolutionSettings::enabled, &ResolutionSettings::setEnabled).category("Resolution");
    type.property("scale", &ResolutionSettings::scale, &ResolutionSettings::setScale).category("Resolution").range(0.25, 1);
    return type.build();
}
std::string ResamplingUpscaler::identifier() const
{
    return "bg2e.draw.bilinear.v1";
}
std::string ResamplingUpscaler::label() const
{
    return "Bilinear reconstruction";
}
UpscalerRequirements ResamplingUpscaler::requirements() const
{
    return {};
}
void ResamplingUpscaler::initialize(Engine& engine, const UpscalerConfiguration& configuration)
{
    gpu::PipelineLayoutDescription layout;
    layout.resourceBindings.push_back({0, {0, 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
    layout.resourceBindings.push_back({0, {1, 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
    layout.pushConstants.push_back({0, 16, gpu::ShaderStage::Fragment});
    _pass.initialize(engine, configuration.shaderRoot, "draw/upscale/resample", engine.device()->createPipelineLayout(layout),
        gpu::PixelFormat::R16G16B16A16_SFLOAT, configuration.frameSlots);
    _ready = true;
}
void ResamplingUpscaler::resize(const UpscalerConfiguration&)
{
}
bool ResamplingUpscaler::available() const
{
    return _ready;
}
std::string ResamplingUpscaler::status() const
{
    return _ready ? "Available" : "Not initialized";
}
bool ResamplingUpscaler::record(const UpscalerInput& input)
{
    const glm::vec4 size{float(input.output.size().width), float(input.output.size().height), 0, 0};
    _pass.draw(input.frame, input.output, {&input.color}, 0, {}, &size, sizeof(size));
    return true;
}
void ResamplingUpscaler::reset()
{
}
void ResamplingUpscaler::cleanup()
{
    _ready = false;
    _pass.cleanup();
}
reflection::TypeInfo ResamplingUpscaler::settingsType() const
{
    reflection::TypeInfoBuilder<ResamplingUpscaler> type(identifier());
    type.property("available", &ResamplingUpscaler::available).category("Capabilities");
    type.property("status", &ResamplingUpscaler::status).category("Capabilities");
    return type.build();
}
}
