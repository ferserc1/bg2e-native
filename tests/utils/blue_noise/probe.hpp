/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/utils/BlueNoise.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <vector>

namespace bg2e::utils::tests {
inline void checkBlueNoiseCpu()
{
    const auto data = BlueNoiseData::embedded();
    data.validate();
    if (data.channels < 2 || data.layers < 2)
    {
        throw std::runtime_error("BlueNoise fixture expects multi-layer/two-channel embedded ranks");
    }
    const auto first = data.sample2D(0,0,0,0);
    if (first != glm::vec2((float(data.bytes[0])+0.5f)/256.0f,(float(data.bytes[1])+0.5f)/256.0f))
    {
        throw std::runtime_error("Unshifted blue-noise samples must use rank-bin centers");
    }
    for (uint32_t layer = 0; layer < data.layers; ++layer)
    {
        const auto corner = data.texel(-1,-1,layer);
        const size_t index = ((size_t(layer) + 1) * data.tileSize.width * data.tileSize.height - 1) * data.channels;
        if (corner[0] != data.bytes[index] || corner[1] != data.bytes[index+1] ||
            corner != data.texel(data.tileSize.width-1, data.tileSize.height-1,layer))
        {
            throw std::runtime_error("BlueNoise layer stride or periodic negative-coordinate wrap differs");
        }
    }
    for (uint32_t frame : {0u,1u,65536u,std::numeric_limits<uint32_t>::max()})
    {
        const auto value = sampleNoise2D(&data,-3,-2,frame,9);
        const auto repeated = sampleNoise2D(&data,-3,-2,frame,9);
        const auto fallback = sampleNoise2D(nullptr,-3,-2,frame,9);
        if (value != repeated || value != data.sample2D(int32_t(data.tileSize.width)-3,int32_t(data.tileSize.height)-2,frame,9) || value.x < 0 || value.y < 0 || value.x >= 1 || value.y >= 1 ||
            fallback.x < 0 || fallback.y < 0 || fallback.x >= 1 || fallback.y >= 1 ||
            fallback != sampleNoise2D(nullptr,-3,-2,frame,9))
        {
            throw std::runtime_error("Noise sequence must be deterministic and remain in [0,1)");
        }
    }
    auto invalid = data;
    invalid.bytes = invalid.bytes.first(invalid.bytes.size()-1);
    bool rejected = false;
    try
    {
        invalid.validate();
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    if (!rejected)
    {
        throw std::runtime_error("Noise byte-count mismatch was accepted");
    }
}

// Manual GPU fixture; false means unsupported format, not a successful run.
// Device and deployed shader root are supplied by a stopped utility caller.
inline bool checkBlueNoiseGpu(gpu::Device& device, gpu::BackendType backend,
                              const std::filesystem::path& shaderRoot)
{
    using namespace gpu;
    const auto format = device.formatCapabilities(PixelFormat::R8G8B8A8_UNORM);
    if (!format.supports(ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::TransferSrc | ImageUsage::TransferDst))
    {
        return false;
    }
    checkBlueNoiseCpu();
    const auto data = BlueNoiseData::embedded();
    BlueNoise noise(device);
    noise.build(data);
    auto retainedImage = noise.image();
    auto retainedSampler = noise.sampler();
    const auto metadata = noise.shaderMetadata();
    if (!noise.isReady() || metadata != glm::uvec4(data.tileSize.width,data.tileSize.height,data.channels,data.layers) ||
        retainedImage->imageType() != ImageType::Image2D || retainedImage->arrayLayers() != 1 ||
        retainedImage->mipLevels() != 1 || retainedImage->height() != data.tileSize.height*data.layers)
    {
        throw std::runtime_error("Noise GPU atlas metadata/type mismatch");
    }
    std::vector<uint8_t> atlas;
    retainedImage->readPixelsRGBA8(atlas,ImageLayout::ShaderReadOnly);
    if (atlas.size() != size_t(data.tileSize.width)*data.tileSize.height*data.layers*4)
    {
        throw std::runtime_error("Noise atlas byte size mismatch");
    }
    for (size_t texel = 0; texel < atlas.size()/4; ++texel)
    {
        for (uint32_t channel = 0; channel < 4; ++channel)
        {
            const uint8_t expected = channel < data.channels ? data.bytes[texel*data.channels+channel] :
                channel == 3 ? 255 : 0;
            if (atlas[texel*4+channel] != expected)
            {
                throw std::runtime_error("Noise atlas lost or reordered rank/channel/layer data");
            }
        }
    }
    // A failed replacement preserves published resources and metadata.
    auto invalid = data;
    invalid.layers = 0;
    bool rejected = false;
    try
    {
        noise.build(invalid);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    if (!rejected || noise.image() != retainedImage || noise.shaderMetadata() != metadata)
    {
        throw std::runtime_error("Noise invalid replacement changed a valid snapshot");
    }
    noise.build(data);
    if (noise.image() == retainedImage)
    {
        throw std::runtime_error("Noise replacement did not publish an independent image");
    }
    noise.cleanup();
    if (noise.isReady() || noise.shaderMetadata() != glm::uvec4(0) || !retainedImage->isValid() || !retainedSampler->isValid())
    {
        throw std::runtime_error("Noise cleanup destroyed exported shared owners");
    }
    // Use the retained old snapshot after replacement/cleanup, independently of
    // any renderer, tracing tables or acceleration support.
    const Size2D extent{data.tileSize.width+3,data.tileSize.height*data.layers+2};
    ImageDescription outputDescription;
    outputDescription.size = extent;
    outputDescription.format = PixelFormat::R8G8B8A8_UNORM;
    outputDescription.usage = ImageUsage::Storage | ImageUsage::TransferSrc;
    auto output = device.createImage(outputDescription);
    struct alignas(16) Uniforms {
        glm::uvec4 metadata;
        glm::uvec4 sequence;
    };
    static_assert(sizeof(Uniforms) == 32);
    const Uniforms uniforms{metadata,{0xfffffff0u,0xffffffffu,0,0}};
    auto buffer = device.createBuffer("noise probe metadata/sequence");
    buffer->createUniformBuffer(uniforms);
    ShaderLib library(shaderRoot / "draw/blue_noise",backend);
    for (bool fallback : {false,true})
    {
        auto shader = library.compute(fallback ? "fallback" : "probe",&device);
        PipelineLayoutDescription description;
        description.resourceBindings = {
            {0,{0,1},ResourceType::UniformBuffer,ShaderStage::Compute},
            {0,{1,0},ResourceType::StorageImage,ShaderStage::Compute}};
        if (!fallback)
        {
            description.resourceBindings.push_back({0,{2,1},ResourceType::SampledImage,ShaderStage::Compute});
            description.resourceBindings.push_back({0,{3,0},ResourceType::Sampler,ShaderStage::Compute});
        }
        auto layout = device.createPipelineLayout(description);
        auto pipeline = device.createComputePipeline({shader.get(),layout.get()});
        auto resources = device.createResourceSet(layout.get(),0);
        resources->setUniformBuffer({0,1},buffer.get());
        resources->setStorageImage({1,0},output.get());
        if (!fallback)
        {
            resources->setSampledImage({2,1},retainedImage.get());
            resources->setSampler({3,0},retainedSampler.get());
        }
        resources->update();
        device.immediateSubmit([&](CommandBuffer* command)
        {
            command->transition(output.get(),ImageLayout::General);
            command->beginCompute();
            command->bindPipeline(pipeline.get());
            command->bindResourceSet(pipeline.get(),0,resources.get());
            command->dispatch((extent.width+15)/16,(extent.height+15)/16,1);
            command->endCompute();
        });
        std::vector<uint8_t> pixels;
        output->readPixelsRGBA8(pixels,ImageLayout::General);
        if (pixels.size() != size_t(extent.width)*extent.height*4)
        {
            throw std::runtime_error("Noise dispatch output extent mismatch");
        }
        for (uint32_t y = 0; y < extent.height; ++y)
        {
            const uint32_t frame = uniforms.sequence.x + y/data.tileSize.height;
            for (uint32_t x = 0; x < extent.width; ++x)
            {
                const auto expected = sampleNoise2D(fallback ? nullptr : &data,int32_t(x)-3,int32_t(y)-2,frame,uniforms.sequence.y);
                const auto offset = (size_t(y)*extent.width+x)*4;
                if (std::abs(int(pixels[offset])-int(std::lround(expected.x*255))) > 1 ||
                    std::abs(int(pixels[offset+1])-int(std::lround(expected.y*255))) > 1 ||
                    pixels[offset+2] != 0 || pixels[offset+3] != 255)
                {
                    throw std::runtime_error("CPU/native noise layer/wrap/sequence/fallback samples differ");
                }
            }
        }
    }
    return true;
}
}
