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

#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace bg2e::bake::detail {
static_assert(sizeof(float) == 4 && sizeof(uint32_t) == 4 && sizeof(glm::uvec4) == 16);
inline gpu::PipelineLayoutDescription outputLayout(uint32_t sampled, uint32_t storage)
{
    gpu::PipelineLayoutDescription layout;
    layout.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
    layout.resourceBindings.push_back({0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
    for (uint32_t i = 0; i < sampled+storage; ++i)
    {
        layout.resourceBindings.push_back({0,{2+i,i},i < sampled ?
            gpu::ResourceType::SampledImage : gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
    }
    return layout;
}

inline std::shared_ptr<gpu::Image> outputImage(gpu::Device& device, gpu::Size2D extent, gpu::PixelFormat format)
{
    gpu::ImageDescription description;
    description.size = extent;
    description.format = format;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    if (extent.isZero() || extent.width > device.renderingCapabilities().maxImageDimension2D ||
        extent.height > device.renderingCapabilities().maxImageDimension2D ||
        !device.formatCapabilities(format).supports(description.usage))
    {
        throw std::runtime_error("Bake output extent/format/usage is unsupported");
    }
    return device.createImage(description);
}

inline void sampledImage(gpu::Device& device, const std::shared_ptr<gpu::Image>& image,
    gpu::Size2D extent, gpu::PixelFormat format)
{
    if (!image || !image->isValid() || image->device() != &device || image->size() != extent ||
        image->imageType() != gpu::ImageType::Image2D || image->pixelFormat() != format ||
        !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
    {
        throw std::invalid_argument("Bake output input requires matching same-device sampled 2D format/extent");
    }
}

inline void dispatch(gpu::CommandBuffer& command, gpu::ComputePipeline& pipeline, gpu::ResourceSet& set,
    gpu::Size2D extent, const std::vector<std::shared_ptr<gpu::Image>>& inputs,
    const std::vector<std::shared_ptr<gpu::Image>>& outputs)
{
    for (const auto& image : inputs)
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    for (const auto& image : outputs)
    {
        command.transition(image.get(),gpu::ImageLayout::General);
    }
    command.beginCompute();
    command.bindPipeline(&pipeline);
    command.bindResourceSet(&pipeline,0,&set);
    command.dispatch((extent.width+15)/16,(extent.height+15)/16,1);
    command.endCompute();
    for (const auto& image : outputs)
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
}

inline size_t pixels(gpu::Size2D extent)
{
    const uint64_t count = uint64_t(extent.width)*extent.height;
    if (extent.isZero() || count > std::numeric_limits<size_t>::max()/16)
    {
        throw std::length_error("Bake readback extent/byte size overflow");
    }
    return size_t(count);
}

template<class T> std::vector<T> read(const std::shared_ptr<gpu::Image>& image, size_t elements)
{
    if (!image || elements > std::numeric_limits<size_t>::max()/sizeof(T))
    {
        throw std::invalid_argument("Missing or overflowing bake readback");
    }
    std::vector<uint8_t> bytes;
    image->readPixels(bytes,image->fullRange(),gpu::ImageLayout::ShaderReadOnly);
    if (bytes.size() != elements*sizeof(T))
    {
        throw std::runtime_error("Bake readback returned unexpected tightly packed byte length");
    }
    std::vector<T> result(elements);
    std::memcpy(result.data(),bytes.data(),bytes.size());
    return result;
}
}
