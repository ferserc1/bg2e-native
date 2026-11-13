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

#pragma once

#include <bg2e/draw/environment/all.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/db/image.hpp>
#include <array>
#include <bg2e/gpu/Device.hpp>

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {
inline float environmentProbeHalf(uint16_t value)
{
    const int exponent = (value >> 10) & 31;
    const int fraction = value & 1023;
    const float sign = (value & 32768) != 0 ? -1.0f : 1.0f;
    if (exponent == 31)
    {
        throw std::runtime_error("Environment output contains nonfinite half pixels");
    }
    return sign * (exponent == 0 ? std::ldexp(float(fraction), -24) : std::ldexp(1.0f + float(fraction) / 1024.0f, exponent - 15));
}

inline void checkConstantEnvironment(gpu::Image& image, float scale)
{
    for (uint32_t mip = 0; mip < image.mipLevels(); ++mip)
    {
        for (uint32_t face = 0; face < image.arrayLayers(); ++face)
        {
            std::vector<uint8_t> bytes;
            image.readPixels(bytes, {mip, 1, face, 1});
            const auto extent = image.mipExtent(mip);
            if (bytes.size() != size_t(extent.width) * extent.height * 8)
            {
                throw std::runtime_error("Unexpected RGBA16F environment readback size");
            }
            for (size_t offset = 0; offset < bytes.size(); offset += 8)
            {
                const float expected[4] = {0.25f * scale, 0.5f * scale, scale, 1};
                for (size_t channel = 0; channel < 4; ++channel)
                {
                    uint16_t half;
                    std::memcpy(&half, bytes.data() + offset + channel * 2, 2);
                    if (std::abs(environmentProbeHalf(half) - expected[channel]) > 0.02f)
                    {
                        throw std::runtime_error("Constant environment energy/mip/face check failed");
                    }
                }
            }
        }
    }
}

// Initialized Engine, producers stopped, no active frame. The caller supplies
// the deployed engine shader root and a path that does not exist.
inline void runEnvironmentProbe(Engine& engine, const std::filesystem::path& shaderRoot,
    const std::filesystem::path& missingHDR)
{
    if (missingHDR.empty() || std::filesystem::exists(missingHDR))
    {
        throw std::invalid_argument("Probe needs a nonempty absent HDR path");
    }
    auto source = engine.device()->createImage({.size = {16, 8}, .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst, .debugName = "Constant linear HDR probe source"});
    // IEEE half encodings: 0.25, 0.5, 1.0, 1.0.
    std::vector<uint16_t> pixels(16 * 8 * 4);
    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i] = 0x3400;
        pixels[i + 1] = 0x3800;
        pixels[i + 2] = pixels[i + 3] = 0x3c00;
    }
    source->upload({.data = pixels.data(), .byteSize = pixels.size() * sizeof(uint16_t)});
    environment::EnvironmentGenerator generator(engine, shaderRoot);
    for (const auto& tool : {generator.configuration().sphere, generator.configuration().defaultSky,
        generator.configuration().irradiance, generator.configuration().specular, generator.configuration().brdf})
    {
        tool->settings().size = 8;
        tool->settings().samples = 64;
    }
    auto first = generator.build(source);
    first->validate(engine.device());
    checkConstantEnvironment(*first->sky, 1);
    checkConstantEnvironment(*first->irradiance, 3.14159265f);
    checkConstantEnvironment(*first->prefilteredSpecular, 1);
    std::vector<uint8_t> lut;
    first->brdf->readPixels(lut, {0, 1, 0, 1});
    if (lut.size() != 8 * 8 * 8)
    {
        throw std::runtime_error("Unexpected BRDF LUT readback size");
    }
    for (size_t i = 0; i < lut.size(); i += 2)
    {
        uint16_t half;
        std::memcpy(&half, lut.data() + i, 2);
        const float value = environmentProbeHalf(half);
        if (value < 0 || value > 1.1f)
        {
            throw std::runtime_error("BRDF LUT is outside its expected finite range");
        }
    }
    bool rejected = false;
    try
    {
        generator.build(missingHDR);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    if (!rejected || generator.resources() != first)
    {
        throw std::runtime_error("Failed HDR replacement discarded the published environment");
    }
    scene::EnvironmentComponent intent;
    generator.synchronize(&intent); // Empty source deliberately generates default sky.
    auto unblurred = generator.resources();
    intent.setSkyboxBlur(0.75f);
    intent.setSkyboxVisible(false);
    if (!generator.synchronize(&intent) || generator.resources()->sky != unblurred->sky ||
        generator.resources()->skyboxBlur != 0.75f || generator.resources()->skyboxVisible)
    {
        throw std::runtime_error("Declarative sky settings rebuilt images or failed to update");
    }
    // Old resource snapshots remain usable after replacement and tool cleanup.
    generator.cleanup();
    first->validate(engine.device());
    checkConstantEnvironment(*first->prefilteredSpecular, 1);
}

// Writes only the caller-supplied absent PNG path and removes it on exit.
// Initialized Engine, producers stopped, no active frame, deployed shaders.
inline void runSdrEnvironmentProbe(Engine& engine, const std::filesystem::path& shaderRoot,
    const std::filesystem::path& pngPath)
{
    if (pngPath.extension() != ".png" || std::filesystem::exists(pngPath))
    {
        throw std::invalid_argument("SDR probe needs an absent .png path");
    }
    struct RemoveImage {
        std::filesystem::path path;
        ~RemoveImage()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    } removeImage{pngPath};
    environment::EnvironmentGenerator generator(engine, shaderRoot);
    for (const auto& tool : {generator.configuration().sphere, generator.configuration().defaultSky,
        generator.configuration().irradiance, generator.configuration().specular, generator.configuration().brdf})
    {
        tool->settings().size = 8;
        tool->settings().samples = 64;
    }
    // Rotate unequal channels and cover both branches of the sRGB transfer.
    const std::array<std::array<uint8_t, 3>, 3> colors{{{8, 128, 255}, {255, 8, 128}, {128, 255, 8}}};
    for (const auto& color : colors)
    {
        std::vector<uint8_t> pixels(16 * 8 * 3);
        std::array<float, 3> linear;
        for (size_t channel = 0; channel < 3; ++channel)
        {
            const float encoded = color[channel] / 255.0f;
            linear[channel] = encoded <= 0.04045f ? encoded / 12.92f : std::pow((encoded + 0.055f) / 1.055f, 2.4f);
        }
        for (size_t i = 0; i < pixels.size(); ++i)
        {
            pixels[i] = color[i % 3];
        }
        db::saveImage(pngPath, pixels.data(), 16, 8, 3);
        auto resources = generator.build(pngPath);
        resources->validate(engine.device());
        for (const auto& image : {resources->sky, resources->irradiance, resources->prefilteredSpecular})
        {
            const float scale = image == resources->irradiance ? 3.14159265f : 1.0f;
            for (uint32_t mip = 0; mip < image->mipLevels(); ++mip)
            {
                for (uint32_t face = 0; face < image->arrayLayers(); ++face)
                {
                    std::vector<uint8_t> bytes;
                    image->readPixels(bytes, {mip, 1, face, 1});
                    const auto extent = image->mipExtent(mip);
                    if (image->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
                        bytes.size() != size_t(extent.width) * extent.height * 8)
                    {
                        throw std::runtime_error("Unexpected SDR environment readback format/size");
                    }
                    for (size_t offset = 0; offset < bytes.size(); offset += 8)
                    {
                        for (size_t channel = 0; channel < 4; ++channel)
                        {
                            uint16_t half;
                            std::memcpy(&half, bytes.data() + offset + channel * 2, 2);
                            const float expected = channel < 3 ? linear[channel] * scale : 1.0f;
                            if (std::abs(environmentProbeHalf(half) - expected) > 0.001f + 0.02f * expected)
                            {
                                throw std::runtime_error("SDR environment channel/linearization/energy regression");
                            }
                        }
                    }
                }
            }
        }
    }
    generator.cleanup();
}

}
