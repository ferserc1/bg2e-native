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

#include <bg2e/render/deferred/TemporalAccumulator.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace bg2e::render::deferred {

// TODO: Refactoring. Extract this function
bool matrixChanged(const glm::mat4& a, const glm::mat4& b, float epsilon)
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (std::abs(a[col][row] - b[col][row]) > epsilon)
            {
                return true;
            }
        }
    }

    return false;
}

TemporalAccumulator::TemporalAccumulator(Engine* engine)
    : _engine{engine}
{
}

TemporalAccumulator::~TemporalAccumulator()
{
    cleanup();
}

void TemporalAccumulator::build(const GBufferManager* gbuffer, VkExtent2D extent)
{
    _extent = extent;

    if (!_engine->rayTracingSupported())
    {
        return;
    }

    _depthFormat = gbuffer->depthFormat();
    _normalFormat = gbuffer->formats()[1];

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    createHistoryImages(extent);
    createPipeline();
}

void TemporalAccumulator::createHistoryImages(VkExtent2D extent)
{
    cleanupImages();

    _historyImagesA.resize(_engine->numImages());
    _historyImagesB.resize(_engine->numImages());
    _prevDepthImages.resize(_engine->numImages());
    _prevNormalImages.resize(_engine->numImages());
    _writeIndex.resize(_engine->numImages(), 0);
    _hasHistory.resize(_engine->numImages(), false);
    _accumulatedFrameCount.resize(_engine->numImages(), 0);
    _previousViewProjection.resize(_engine->numImages(), glm::mat4(1.0f));

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd)
    {
        for (uint32_t i = 0; i < _engine->numImages(); i++)
        {
            _historyImagesA[i] = std::shared_ptr<vulkan::Image>(
                vulkan::Image::createAllocatedImage(
                    _engine,
                    "Temporal history image A " + std::to_string(i),
                    VK_FORMAT_R16_SFLOAT,
                    extent,
                    VK_IMAGE_USAGE_STORAGE_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    1, false, 0, VK_SAMPLE_COUNT_1_BIT
                )
            );

            _historyImagesB[i] = std::shared_ptr<vulkan::Image>(
                vulkan::Image::createAllocatedImage(
                    _engine,
                    "Temporal history image B " + std::to_string(i),
                    VK_FORMAT_R16_SFLOAT,
                    extent,
                    VK_IMAGE_USAGE_STORAGE_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    1, false, 0, VK_SAMPLE_COUNT_1_BIT
                )
            );

            vulkan::Image::cmdTransitionImage(
                cmd,
                _historyImagesB[i]->handle(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            _prevDepthImages[i] = std::shared_ptr<vulkan::Image>(
                vulkan::Image::createAllocatedImage(
                    _engine,
                    "Temporal prev depth " + std::to_string(i),
                    _depthFormat,
                    extent,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    1, false, 0, VK_SAMPLE_COUNT_1_BIT
                )
            );

            _prevNormalImages[i] = std::shared_ptr<vulkan::Image>(
                vulkan::Image::createAllocatedImage(
                    _engine,
                    "Temporal prev normal " + std::to_string(i),
                    _normalFormat,
                    extent,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    1, false, 0, VK_SAMPLE_COUNT_1_BIT
                )
            );

            vulkan::Image::TransitionInfo depthTi;
            depthTi.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vulkan::Image::cmdTransitionImage(cmd, _prevDepthImages[i]->handle(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthTi);

            vulkan::Image::cmdTransitionImage(cmd, _prevNormalImages[i]->handle(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });
}

void TemporalAccumulator::createPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(AccumulatorPushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT
    );
    _pipelineLayout = layoutFactory.build();

    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader("temporal_accumulation.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void TemporalAccumulator::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const GBufferManager* gbuffer,
    const vulkan::Image* aoImage,
    const glm::mat4& currentInverseViewProjection,
    const glm::mat4& currentView,
    const glm::mat4& currentProjection
)
{
    if (!_pipeline) return;

    uint32_t frameIndex = _engine->currentFrameResourcesIndex();

    auto frameViewProj = currentProjection * currentView;
    bool cameraChanged = _hasHistory[frameIndex] &&
            matrixChanged(
                _previousViewProjection[frameIndex],
                frameViewProj,
                0.001f
                );

    if (cameraChanged)
    {
        _hasHistory[frameIndex] = false;
        _accumulatedFrameCount[frameIndex] = 0;
    }

    auto historyRead = historyReadImage(frameIndex);
    auto historyWrite = historyWriteImage(frameIndex);

    vulkan::Image::cmdTransitionImage(cmd, historyWrite->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        aoImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        historyRead.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        historyWrite.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->addImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        _prevDepthImages[frameIndex].get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        _prevNormalImages[frameIndex].get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    AccumulatorPushConstants pc{};
    pc.currentInverseViewProjection = currentInverseViewProjection;
    pc.previousViewProjection = _previousViewProjection[frameIndex];
    pc.outputSize = glm::vec2(static_cast<float>(_extent.width), static_cast<float>(_extent.height));
    pc.historyWeight = _historyWeight;
    pc.accumulatedFrameCount = _accumulatedFrameCount[frameIndex];
    pc.useProgressiveMode = _accumulationMode == AccumulationMode::Progressive ? 1u : 0u;
    pc.hasHistory = _hasHistory[frameIndex] ? 1u : 0u;
    pc.depthThreshold = _depthThreshold;
    pc.normalThreshold = _normalThreshold;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AccumulatorPushConstants), &pc);

    uint32_t groupX = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
    uint32_t groupY = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
    vkCmdDispatch(cmd, groupX, groupY, 1);

    vulkan::Image::cmdTransitionImage(cmd, historyWrite->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Copy current g-buffer depth → prev depth
    {
        vulkan::Image::TransitionInfo depthTi;
        depthTi.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        vulkan::Image::cmdTransitionImage(cmd,
            gbuffer->depthImage()->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthTi);

        vulkan::Image::cmdTransitionImage(cmd,
            _prevDepthImages[frameIndex]->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, depthTi);

        VkImageCopy depthRegion{};
        depthRegion.srcSubresource = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1 };
        depthRegion.dstSubresource = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1 };
        depthRegion.extent = gbuffer->depthImage()->extent();
        vkCmdCopyImage(cmd,
            gbuffer->depthImage()->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _prevDepthImages[frameIndex]->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &depthRegion);

        vulkan::Image::cmdTransitionImage(cmd,
            gbuffer->depthImage()->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthTi);

        vulkan::Image::cmdTransitionImage(cmd,
            _prevDepthImages[frameIndex]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthTi);
    }

    // Copy current g-buffer normal → prev normal
    {
        vulkan::Image::cmdTransitionImage(cmd,
            gbuffer->image(1)->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vulkan::Image::cmdTransitionImage(cmd,
            _prevNormalImages[frameIndex]->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkImageCopy normalRegion{};
        normalRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        normalRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        normalRegion.extent = gbuffer->image(1)->extent();
        vkCmdCopyImage(cmd,
            gbuffer->image(1)->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _prevNormalImages[frameIndex]->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &normalRegion);

        vulkan::Image::cmdTransitionImage(cmd,
            gbuffer->image(1)->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vulkan::Image::cmdTransitionImage(cmd,
            _prevNormalImages[frameIndex]->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    uint32_t newIndex = 1 - _writeIndex[frameIndex];
    _writeIndex[frameIndex] = newIndex;

    if (!_hasHistory[frameIndex])
    {
        _hasHistory[frameIndex] = true;
    }
    _accumulatedFrameCount[frameIndex]++;

    _previousViewProjection[frameIndex] = frameViewProj;
}

void TemporalAccumulator::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createHistoryImages(newExtent);
}

void TemporalAccumulator::cleanup()
{
    cleanupImages();

    if (_pipeline)
    {
        vkDestroyPipeline(_engine->device().handle(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_dsLayout)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    }
    if (_sampler)
    {
        vkDestroySampler(_engine->device().handle(), _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
}

void TemporalAccumulator::cleanupImages()
{
    for (auto& img : _historyImagesA)
    {
        if (img) img->cleanup();
    }
    _historyImagesA.clear();
    for (auto& img : _historyImagesB)
    {
        if (img) img->cleanup();
    }
    _historyImagesB.clear();
    for (auto& img : _prevDepthImages)
    {
        if (img) img->cleanup();
    }
    _prevDepthImages.clear();
    for (auto& img : _prevNormalImages)
    {
        if (img) img->cleanup();
    }
    _prevNormalImages.clear();
    _writeIndex.clear();
    _hasHistory.clear();
    _accumulatedFrameCount.clear();
    _previousViewProjection.clear();
}

std::shared_ptr<vulkan::Image> TemporalAccumulator::outputImage(uint32_t frameIndex) const
{
    return historyReadImage(frameIndex);
}

VkSampler TemporalAccumulator::sampler() const
{
    return _sampler;
}

void TemporalAccumulator::invalidateHistory()
{
    for (uint32_t i = 0; i < _hasHistory.size(); i++)
    {
        _hasHistory[i] = false;
        _accumulatedFrameCount[i] = 0;
    }
}

void TemporalAccumulator::setAccumulationMode(AccumulationMode mode)
{
    _accumulationMode = mode;
}

TemporalAccumulator::AccumulationMode TemporalAccumulator::accumulationMode() const
{
    return _accumulationMode;
}

void TemporalAccumulator::setHistoryWeight(float weight)
{
    _historyWeight = weight;
}

float TemporalAccumulator::historyWeight() const
{
    return _historyWeight;
}

void TemporalAccumulator::setDepthThreshold(float threshold)
{
    _depthThreshold = threshold;
}

void TemporalAccumulator::setNormalThreshold(float threshold)
{
    _normalThreshold = threshold;
}

float TemporalAccumulator::depthThreshold() const
{
    return _depthThreshold;
}

float TemporalAccumulator::normalThreshold() const
{
    return _normalThreshold;
}

std::shared_ptr<vulkan::Image> TemporalAccumulator::historyReadImage(uint32_t frameIndex) const
{
    uint32_t readIndex = 1 - _writeIndex[frameIndex];
    if (readIndex == 0)
    {
        return _historyImagesA[frameIndex];
    }
    return _historyImagesB[frameIndex];
}

std::shared_ptr<vulkan::Image> TemporalAccumulator::historyWriteImage(uint32_t frameIndex) const
{
    if (_writeIndex[frameIndex] == 0)
    {
        return _historyImagesA[frameIndex];
    }
    return _historyImagesB[frameIndex];
}

}