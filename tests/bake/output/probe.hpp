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

#include <bg2e/bake/all.hpp>
#include <bg2e/gpu/Device.hpp>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace bg2e::bake::tests {
inline void runBakePixelConversionProbe()
{
    const std::vector<float> rgba{2,0,0.25f,1,0.5f,1,0,1};
    const auto hdr = LightmapCompositionPass::convertRgba32({2,1},rgba,
        LightmapMode::RTGI,LightmapPixelFormat::RGB32F,16);
    const auto linear = std::get<std::vector<float>>(hdr.rgb);
    if (linear != std::vector<float>{2,0,0.25f,0.5f,1,0})
    {
        throw std::runtime_error("Float bake export must preserve HDR/black/row order and ignore exposure");
    }
    const auto bytes = LightmapCompositionPass::convertRgba32({2,1},rgba,
        LightmapMode::RTGI,LightmapPixelFormat::RGB8,-1);
    if (std::get<std::vector<uint8_t>>(bytes.rgb) != std::vector<uint8_t>{255,0,32,64,128,0})
    {
        throw std::runtime_error("GI byte export must apply linear exposure, clamp and round once");
    }
    const auto ao = LightmapCompositionPass::convertRgba32({1,1},{0.25f,0.25f,0.25f,1},
        LightmapMode::RTAO,LightmapPixelFormat::RGB8,16);
    if (std::get<std::vector<uint8_t>>(ao.rgb) != std::vector<uint8_t>{64,64,64})
    {
        throw std::runtime_error("AO byte export must ignore exposure");
    }
    for (float invalid : {-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
    {
        bool failed = false;
        try
        {
            LightmapCompositionPass::convertRgba32({1,1},{invalid,0,0,1},
                LightmapMode::RTGI,LightmapPixelFormat::RGB8,0);
        }
        catch (const std::exception&)
        {
            failed = true;
        }
        if (!failed)
        {
            throw std::runtime_error("Byte conversion must diagnose invalid float output before clamping");
        }
    }
}

// No RT needed: synthetic raw AO samples with adjacent charts and identical world
// guides prove chart ownership independent of world-position/normal similarity.
inline void runBakeOutputGpuProbe(gpu::Device& device, gpu::BackendType backend,
    const std::filesystem::path& shaderRoot)
{
    const gpu::Size2D extent{9,9};
    constexpr size_t count = 81;
    auto image = [&](gpu::PixelFormat format, const void* data, size_t bytes)
    {
        gpu::ImageDescription description;
        description.size = extent;
        description.format = format;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
        auto result = device.createImage(description);
        result->upload({data,bytes});
        return result;
    };
    std::vector<float> positions(count*4,0);
    std::vector<uint16_t> normals(count*4,0);
    std::vector<uint32_t> charts(count,0), submeshes(count,UINT32_MAX);
    std::vector<uint8_t> coverage(count,0);
    const size_t left = 2*9+2, right = 2*9+6;
    charts[left] = 1;
    charts[right] = 2;
    for (auto i : {left,right})
    {
        coverage[i] = 255;
        submeshes[i] = 0;
        normals[i*4+2] = 0x3c00; // Half-float +Z and alpha one.
        normals[i*4+3] = 0x3c00;
        positions[i*4+3] = 1;
    }
    BakeSample sample;
    sample.mode = LightmapMode::RTAO;
    sample.surface.geometry = std::make_shared<UvSurfaceGeometry>();
    sample.surface.sourceGeneration = 7;
    sample.surface.targetGeneration = 11;
    sample.surface.images = {image(gpu::PixelFormat::R32G32B32A32_SFLOAT,positions.data(),positions.size()*4),
        image(gpu::PixelFormat::R16G16B16A16_SFLOAT,normals.data(),normals.size()*2),
        image(gpu::PixelFormat::R32_UINT,submeshes.data(),submeshes.size()*4),
        image(gpu::PixelFormat::R32_UINT,charts.data(),charts.size()*4),
        image(gpu::PixelFormat::R8_UNORM,coverage.data(),coverage.size())};
    std::vector<uint16_t> raw0(count,0), raw1(count,0);
    raw0[right] = 0x3c00; // First: left zero, right one.
    raw1[left] = raw1[right] = 0x3800; // Second: both half.
    const auto signal0 = image(gpu::PixelFormat::R16_SFLOAT,raw0.data(),raw0.size()*2);
    const auto signal1 = image(gpu::PixelFormat::R16_SFLOAT,raw1.data(),raw1.size()*2);
    AccumulationConfiguration ac;
    ac.shaderRoot = shaderRoot;
    UvTemporalAccumulator accumulator(ac);
    accumulator.initialize(device,backend,2);
    CompositionConfiguration cc;
    cc.shaderRoot = shaderRoot;
    LightmapCompositionPass composition(cc);
    composition.initialize(device,backend,2);
    LightmapSettings settings;
    settings.setMode(LightmapMode::RTAO);
    settings.setResolution(9);
    settings.setDilationPixels(2);
    settings.setAccumulationFrames(2);
    settings.setCpuFormat(LightmapPixelFormat::RGB32F);
    std::shared_ptr<const AccumulationResult> first, second;
    sample.signal = signal0;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        first = accumulator.record(*command,0,1,sample);
    });
    sample.sampleIndex = 1;
    sample.signal = signal1;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        second = accumulator.record(*command,1,2,sample,first);
    });
    UvTemporalAccumulator::validateCompleted(*second,2);
    CompositionResult result;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        result = composition.record(*command,1,2,*second,settings,nullptr);
    });
    const auto pixels = LightmapCompositionPass::readPixels(result,settings);
    const auto& rgb = std::get<std::vector<float>>(pixels.rgb);
    if (rgb[left*3] != 0.25f || rgb[right*3] != 0.75f || rgb[(2*9+4)*3] != 0.25f ||
        rgb[(0*9+2)*3] != 0.25f || rgb[0] != 1 || rgb[(4*9+4)*3] != 1)
    {
        throw std::runtime_error("AO mean, fixed chart interior, nearest seed tie, radius or neutral padding mismatch");
    }
    settings.setDilationPixels(0);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        result = composition.record(*command,1,3,*second,settings,nullptr);
    });
    const auto undilated = LightmapCompositionPass::readPixels(result,settings);
    if (std::get<std::vector<float>>(undilated.rgb)[(2*9+4)*3] != 1)
    {
        throw std::runtime_error("Zero dilation must leave uncovered atlas texels neutral");
    }
    std::vector<uint8_t> sumsBytes;
    second->sums->readPixels(sumsBytes,second->sums->fullRange());
    float leftSum = 0, rightSum = 0;
    std::memcpy(&leftSum,sumsBytes.data()+left*16,4);
    std::memcpy(&rightSum,sumsBytes.data()+right*16,4);
    if (leftSum != 0.5f || rightSum != 1.5f || first->updates != 1 || second->updates != 2)
    {
        throw std::runtime_error("Raw AO accumulation lost exact valid counts/sums or changed retained history");
    }
    composition.cleanup();
    accumulator.cleanup();
}
// Supply any valid immutable environment for the unused sky/prefilter/BRDF owners.
// The reference irradiance is replaced by a known constant cube, independent of RT.
inline void runBakeGiCompositionProbe(gpu::Device& device, gpu::BackendType backend,
    const std::filesystem::path& shaderRoot,
    std::shared_ptr<const draw::environment::EnvironmentResources> sourceEnvironment)
{
    if (!sourceEnvironment)
    {
        throw std::invalid_argument("GI composition fixture requires valid environment owners");
    }
    sourceEnvironment->validate(&device);
    auto environment = std::make_shared<draw::environment::EnvironmentResources>(*sourceEnvironment);
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    environment->sampler = device.createSampler(sampler);
    gpu::ImageDescription description;
    description.size = {1,1};
    description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
    description.type = gpu::ImageType::Cubemap;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    environment->irradiance = device.createImage(description);
    const std::array<float,4> reference{2*3.14159265359f,4*3.14159265359f,0,1};
    for (uint32_t face = 0; face < 6; ++face)
    {
        gpu::ImageUploadDescription upload;
        upload.data = reference.data();
        upload.byteSize = sizeof(reference);
        upload.arrayLayer = face;
        environment->irradiance->upload(upload);
    }
    auto image = [&](gpu::PixelFormat format, const void* data, size_t bytes)
    {
        gpu::ImageDescription desc;
        desc.size = {1,1};
        desc.format = format;
        desc.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
        auto value = device.createImage(desc);
        value->upload({data,bytes});
        return value;
    };
    const std::array<float,4> sum{4,2,8,0}, position{0,0,0,1};
    const std::array<uint16_t,4> normal{0,0,0x3c00,0x3c00};
    const uint32_t one = 1;
    const uint8_t full = 255;
    AccumulationResult accumulation;
    accumulation.mode = LightmapMode::RTGI;
    accumulation.updates = 1;
    accumulation.sums = image(gpu::PixelFormat::R32G32B32A32_SFLOAT,sum.data(),sizeof(sum));
    accumulation.counts = image(gpu::PixelFormat::R32_UINT,&one,sizeof(one));
    accumulation.surface.images[0] = image(gpu::PixelFormat::R32G32B32A32_SFLOAT,position.data(),sizeof(position));
    accumulation.surface.images[1] = image(gpu::PixelFormat::R16G16B16A16_SFLOAT,normal.data(),sizeof(normal));
    accumulation.surface.images[3] = image(gpu::PixelFormat::R32_UINT,&one,sizeof(one));
    accumulation.surface.images[4] = image(gpu::PixelFormat::R8_UNORM,&full,sizeof(full));
    UvTemporalAccumulator::validateCompleted(accumulation,1);
    LightmapSettings settings;
    settings.setResolution(1);
    settings.setDilationPixels(0);
    settings.setAccumulationFrames(1);
    settings.setCpuFormat(LightmapPixelFormat::RGB32F);
    CompositionConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    LightmapCompositionPass pass(configuration);
    pass.initialize(device,backend,1);
    CompositionResult result;
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        result = pass.record(*command,0,1,accumulation,settings,environment);
    });
    const auto pixels = LightmapCompositionPass::readPixels(result,settings);
    const auto& rgb = std::get<std::vector<float>>(pixels.rgb);
    if (std::abs(rgb[0]-2.0f) > 1e-5f || std::abs(rgb[1]-0.5f) > 1e-5f || rgb[2] != 0)
    {
        throw std::runtime_error("GI normalization must divide by irradiance/pi once, preserve HDR and zero reference");
    }
    settings.setCpuFormat(LightmapPixelFormat::RGB8);
    settings.setExposureEV(-1);
    const auto bytes = LightmapCompositionPass::readPixels(result,settings);
    if (std::get<std::vector<uint8_t>>(bytes.rgb) != std::vector<uint8_t>{255,64,0})
    {
        throw std::runtime_error("Completed GI GPU image and exposed linear byte readback differ");
    }
    pass.cleanup();
}

}
