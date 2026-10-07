#include "UvTemporalAccumulator.hpp"

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace bg2e::render {

UvTemporalAccumulator::UvTemporalAccumulator(Engine* engine, VkExtent2D extent)
    : _engine(engine), _extent(extent)
{
    if (!engine || extent.width == 0 || extent.height == 0)
    {
        throw std::invalid_argument("UvTemporalAccumulator: invalid engine or extent");
    }
    for (uint32_t i = 0; i < 2; ++i)
    {
        _history[i].reset(vulkan::Image::createAllocatedImage(
            engine, "UV bake history " + std::to_string(i),
            VK_FORMAT_R32G32B32A32_SFLOAT, extent,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
    }

    vulkan::factory::Sampler samplerFactory(engine);
    samplerFactory.createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    _sampler = samplerFactory.build(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

    vulkan::factory::DescriptorSetLayout layoutFactory;
    layoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // composed sample
    layoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // previous history
    layoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // valid texels
    layoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);          // next history
    _descriptorSetLayout = layoutFactory.build(engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);

    vulkan::factory::PipelineLayout pipelineLayoutFactory(engine);
    pipelineLayoutFactory.addDescriptorSetLayout(_descriptorSetLayout);
    pipelineLayoutFactory.addPushConstantRange(0, sizeof(uint32_t), VK_SHADER_STAGE_COMPUTE_BIT);
    _pipelineLayout = pipelineLayoutFactory.build("UvTemporalAccumulator::PipelineLayout");

    vulkan::factory::ComputePipeline pipelineFactory(engine);
    pipelineFactory.setShader("lightmap_accumulation.comp.spv");
    _pipeline = pipelineFactory.build(_pipelineLayout, "UvTemporalAccumulator::Pipeline");
}

UvTemporalAccumulator::~UvTemporalAccumulator()
{
    const VkDevice device = _engine->device().handle();
    if (_pipeline) vkDestroyPipeline(device, _pipeline, nullptr);
    if (_pipelineLayout) vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
    if (_descriptorSetLayout) vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
    if (_sampler) vkDestroySampler(device, _sampler, nullptr);
}

void UvTemporalAccumulator::record(VkCommandBuffer cmd,
                                   vulkan::DescriptorSetAllocator& descriptors,
                                   const vulkan::Image& sample,
                                   const GBufferManager& uvSurface,
                                   uint32_t previousSamples)
{
    if (sample.extent2D().width != _extent.width || sample.extent2D().height != _extent.height ||
        uvSurface.extent().width != _extent.width || uvSurface.extent().height != _extent.height ||
        uvSurface.imageCount() < 4)
    {
        throw std::invalid_argument("UvTemporalAccumulator: sample and mask must match the atlas extent");
    }
    const uint32_t next = _hasResult ? 1u - _active : 0u;
    auto& output = *_history[next];
    vulkan::Image::cmdTransitionImage(cmd, output.handle(),
        _initialized[next] ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL);

    std::unique_ptr<vulkan::DescriptorSet> set(descriptors.allocate(_descriptorSetLayout));
    set->beginUpdate();
    set->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        &sample, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    // The previous image is not read when previousSamples == 0, but a valid
    // image descriptor is still required. The freshly written output has a
    // valid view and remains untouched by the previous-history branch.
    set->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        previousSamples ? _history[_active].get() : &sample,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    set->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        uvSurface.image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    set->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &output, VK_IMAGE_LAYOUT_GENERAL);
    set->endUpdate();

    const VkDescriptorSet rawSet = set->descriptorSet();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &rawSet, 0, nullptr);
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(previousSamples), &previousSamples);
    vkCmdDispatch(cmd, (_extent.width + 7) / 8, (_extent.height + 7) / 8, 1);
    vulkan::Image::cmdTransitionImage(cmd, output.handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _initialized[next] = true;
    _active = next;
    _hasResult = true;
}

void UvTemporalAccumulator::reset()
{
    // The next dispatch does not read old history; no GPU clear is needed.
    _hasResult = false;
}

std::shared_ptr<vulkan::Image> UvTemporalAccumulator::image() const
{
    if (!_hasResult) throw std::logic_error("UvTemporalAccumulator: no result is available");
    return _history[_active];
}

LightmapPixels UvTemporalAccumulator::readPixels(LightmapPixelFormat format, float exposureEV) const
{
    if (!std::isfinite(exposureEV) || exposureEV < -16.0f || exposureEV > 16.0f)
    {
        throw std::invalid_argument("UvTemporalAccumulator: exposureEV must be finite and within [-16, 16]");
    }
    auto resultImage = image();
    const size_t pixelCount = static_cast<size_t>(_extent.width) * _extent.height;
    if (pixelCount > std::numeric_limits<size_t>::max() / (sizeof(float) * 4))
    {
        throw std::overflow_error("UvTemporalAccumulator: readback extent is too large");
    }
    const VkDeviceSize byteCount = pixelCount * sizeof(float) * 4;
    std::unique_ptr<vulkan::Buffer> staging(vulkan::Buffer::createAllocatedBuffer(
        _engine, byteCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_TO_CPU, "UV bake RGB readback"));

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        vulkan::Image::cmdTransitionImage(cmd, resultImage->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {_extent.width, _extent.height, 1};
        vkCmdCopyImageToBuffer(cmd, resultImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            staging->handle(), 1, &copy);
        vulkan::Image::cmdTransitionImage(cmd, resultImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
    VK_ASSERT(vmaInvalidateAllocation(_engine->allocator(), staging->allocation(), 0, VK_WHOLE_SIZE));

    LightmapPixels pixels;
    pixels.width = _extent.width;
    pixels.height = _extent.height;
    pixels.format = format;
    const auto* rgba = static_cast<const float*>(staging->allocatedData());
    if (format == LightmapPixelFormat::RGB32F)
    {
        auto& rgb = pixels.rgb.emplace<std::vector<float>>(pixelCount * 3);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            std::memcpy(rgb.data() + i * 3, rgba + i * 4, sizeof(float) * 3);
        }
    }
    else if (format == LightmapPixelFormat::RGB8)
    {
        const float exposure = std::exp2(exposureEV);
        auto& rgb = pixels.rgb.emplace<std::vector<uint8_t>>(pixelCount * 3);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            for (size_t channel = 0; channel < 3; ++channel)
            {
                const float source = rgba[i * 4 + channel];
                const float value = std::isfinite(source) ? source * exposure : 0.0f;
                rgb[i * 3 + channel] = static_cast<uint8_t>(
                    std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
            }
        }
    }
    else
    {
        throw std::invalid_argument("UvTemporalAccumulator: unsupported CPU pixel format");
    }
    return pixels;
}

}
