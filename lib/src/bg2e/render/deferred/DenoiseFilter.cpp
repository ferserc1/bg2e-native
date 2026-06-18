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

#include <bg2e/render/deferred/DenoiseFilter.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <cmath>

namespace bg2e::render::deferred {

DenoiseFilter::DenoiseFilter(Engine * engine)
    : _engine{engine}
{
}

DenoiseFilter::~DenoiseFilter()
{
    cleanup();
}

void DenoiseFilter::build(const GBufferManager * gbuffer, VkExtent2D extent)
{
    _extent = extent;

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    createOutputImages(extent);
    createPipeline();
}

void DenoiseFilter::createOutputImages(VkExtent2D extent)
{
    cleanupImages();

    VkFormat fmt = _isHDR ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8_UNORM;

    _outputImages.resize(_engine->numImages());
    for (uint32_t i = 0; i < _outputImages.size(); i++)
    {
        _outputImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "Denoise output image " + std::to_string(i),
                fmt,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}

void DenoiseFilter::createPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(DenoisePushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT
    );
    _pipelineLayout = layoutFactory.build("DenoiseFilter::PipelineLayout");

    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader(_isHDR ? "denoise_bilateral_hdr.comp.spv" : "denoise_bilateral.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout, "DenoiseFilter::Pipeline");

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void DenoiseFilter::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources & frameResources,
    const GBufferManager * gbuffer,
    const vulkan::Image * inputImage
)
{
    auto outputImage = _outputImages[_engine->currentFrameResourcesIndex()];

    vulkan::Image::cmdTransitionImage(cmd, outputImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        outputImage.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->endUpdate();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    DenoisePushConstants pc{};
    pc.outputSize = glm::vec2(static_cast<float>(_extent.width), static_cast<float>(_extent.height));
    pc.kernelRadius = _kernelRadius;
    pc.depthThreshold = _depthThreshold;
    pc.normalThreshold = _normalThreshold;
    pc.depthSigma = _depthSigma;
    pc.normalSigma = _normalSigma;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DenoisePushConstants), &pc);

    uint32_t groupX = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
    uint32_t groupY = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
    vkCmdDispatch(cmd, groupX, groupY, 1);

    vulkan::Image::cmdTransitionImage(cmd, outputImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void DenoiseFilter::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createOutputImages(newExtent);
}

void DenoiseFilter::cleanup()
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

void DenoiseFilter::cleanupImages()
{
    for (auto& img : _outputImages)
    {
        if (img) img->cleanup();
    }
    _outputImages.clear();
}

void DenoiseFilter::setKernelRadius(int radius) { _kernelRadius = radius; }
void DenoiseFilter::setDepthThreshold(float threshold) { _depthThreshold = threshold; }
void DenoiseFilter::setNormalThreshold(float threshold) { _normalThreshold = threshold; }
void DenoiseFilter::setDepthSigma(float sigma) { _depthSigma = sigma; }
void DenoiseFilter::setNormalSigma(float sigma) { _normalSigma = sigma; }

int DenoiseFilter::kernelRadius() const { return _kernelRadius; }
float DenoiseFilter::depthThreshold() const { return _depthThreshold; }
float DenoiseFilter::normalThreshold() const { return _normalThreshold; }
float DenoiseFilter::depthSigma() const { return _depthSigma; }
float DenoiseFilter::normalSigma() const { return _normalSigma; }

std::shared_ptr<vulkan::Image> DenoiseFilter::outputImage(uint32_t frameIndex) const
{
    return _outputImages[frameIndex];
}

VkSampler DenoiseFilter::sampler() const
{
    return _sampler;
}

}