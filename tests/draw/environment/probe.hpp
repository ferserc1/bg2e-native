#pragma once

#include <bg2e/draw/environment/all.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
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
}
