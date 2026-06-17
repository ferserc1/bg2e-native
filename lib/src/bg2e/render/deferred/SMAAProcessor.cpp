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

#include <bg2e/render/deferred/SMAAProcessor.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <cmath>

#include <AreaTex.h>
#include <SearchTex.h>

namespace bg2e::render::deferred {

SMAAProcessor::SMAAProcessor(Engine * engine)
    : _engine{engine}
{
}

SMAAProcessor::~SMAAProcessor()
{
    cleanup();
}

void SMAAProcessor::build(VkExtent2D extent, VkFormat outputFormat)
{
    _extent = extent;
    _outputFormat = outputFormat;

    vulkan::factory::Sampler samplerFactory(_engine);
    VkSampler sampler = samplerFactory.build(
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    );

    _sampler = sampler;

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
    });

    loadLUTTextures();
    createPipelines();
    createDescriptorAllocators();
    createFrameImages(extent);
}

void SMAAProcessor::loadLUTTextures()
{
    // Upload the official precomputed SMAA lookup tables (iryku_smaa).
    // AreaTex is R8G8 (160x560); SearchTex is R8 (64x16). Both are left in
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL by the data-upload overload.
    _areaTexture = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine, "SMAA area texture",
            const_cast<unsigned char*>(areaTexBytes),
            { AREATEX_WIDTH, AREATEX_HEIGHT },
            2, // R + G components
            VK_FORMAT_R8G8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, false
        )
    );

    _searchTexture = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine, "SMAA search texture",
            const_cast<unsigned char*>(searchTexBytes),
            { SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT },
            1, // R component
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, false
        )
    );
}

void SMAAProcessor::createPipelines()
{
    // --- Edge detection pipeline ---
    {
        vulkan::factory::DescriptorSetLayout dsLayoutFactory;
        dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _edgeDSLayout = dsLayoutFactory.build(
            _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT
        );

        vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(_edgeDSLayout);
        layoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
        _edgePipelineLayout = layoutFactory.build("SMAAProcessor::EdgePipelineLayout");

        vulkan::factory::ComputePipeline plFactory(_engine);
        plFactory.setShader("smaa_edge_detection.comp.spv");
        _edgeDetectionPipeline = plFactory.build(_edgePipelineLayout, "SMAAProcessor::EdgePipeline");
    }

    // --- Blend weight pipeline ---
    {
        vulkan::factory::DescriptorSetLayout dsLayoutFactory;
        dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _blendWeightDSLayout = dsLayoutFactory.build(
            _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT
        );

        vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(_blendWeightDSLayout);
        layoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
        _blendPipelineLayout = layoutFactory.build("SMAAProcessor::BlendPipelineLayout");

        vulkan::factory::ComputePipeline plFactory(_engine);
        plFactory.setShader("smaa_blend_weight.comp.spv");
        _blendWeightPipeline = plFactory.build(_blendPipelineLayout, "SMAAProcessor::BlendPipeline");
    }

    // --- Neighborhood blend pipeline ---
    {
        vulkan::factory::DescriptorSetLayout dsLayoutFactory;
        dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _neighborhoodBlendDSLayout = dsLayoutFactory.build(
            _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT
        );

        vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(_neighborhoodBlendDSLayout);
        layoutFactory.addPushConstantRange(0, sizeof(SMAAPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
        _nblendPipelineLayout = layoutFactory.build("SMAAProcessor::NBlendPipelineLayout");

        vulkan::factory::ComputePipeline plFactory(_engine);
        plFactory.setShader("smaa_neighborhood_blend.comp.spv");
        _neighborhoodBlendPipeline = plFactory.build(_nblendPipelineLayout, "SMAAProcessor::NBlendPipeline");
    }

    _engine->cleanupManager().push([this](VkDevice dev) {
        vkDestroyPipeline(dev, _edgeDetectionPipeline, nullptr);
        _edgeDetectionPipeline = VK_NULL_HANDLE;
        vkDestroyPipeline(dev, _blendWeightPipeline, nullptr);
        _blendWeightPipeline = VK_NULL_HANDLE;
        vkDestroyPipeline(dev, _neighborhoodBlendPipeline, nullptr);
        _neighborhoodBlendPipeline = VK_NULL_HANDLE;

        vkDestroyPipelineLayout(dev, _edgePipelineLayout, nullptr);
        _edgePipelineLayout = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _blendPipelineLayout, nullptr);
        _blendPipelineLayout = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _nblendPipelineLayout, nullptr);
        _nblendPipelineLayout = VK_NULL_HANDLE;

        vkDestroyDescriptorSetLayout(dev, _edgeDSLayout, nullptr);
        _edgeDSLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _blendWeightDSLayout, nullptr);
        _blendWeightDSLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _neighborhoodBlendDSLayout, nullptr);
        _neighborhoodBlendDSLayout = VK_NULL_HANDLE;
    });
}

void SMAAProcessor::createFrameImages(VkExtent2D extent)
{
    cleanupFrameImages();

    _frames.resize(_engine->numImages());
    for (uint32_t i = 0; i < _frames.size(); i++)
    {
        _frames[i].edgesImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine, "SMAA edges image " + std::to_string(i), VK_FORMAT_R8G8_UNORM,
                extent, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, 1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );

        _frames[i].blendWeightsImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine, "SMAA blend weights image " + std::to_string(i), VK_FORMAT_R8G8B8A8_UNORM,
                extent, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, 1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );

        _frames[i].outputImage = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine, "SMAA output image " + std::to_string(i), _outputFormat,
                extent, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, 1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}

void SMAAProcessor::createDescriptorAllocators()
{
    destroyAllocators();

    _descriptorAllocators.resize(_engine->numImages(), nullptr);

    auto createAllocator = [this](uint32_t i) {
        auto alloc = new vulkan::DescriptorSetAllocator();
        alloc->init(_engine);

        alloc->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0.5f },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0.5f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.5f }
        });

        // Need: edge (1 storage + 1 sampled), blend (1 sampled + 1 storage), nblend (1 sampled + 1 storage)
        // Per pass: need storage for output + sampled for inputs (area + search textures shared)
        alloc->requirePoolSizeRatio(_engine->numImages(), {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0.5f },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0.5f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.5f }
        });

        alloc->initPool();
        return alloc;
    };

    for (uint32_t i = 0; i < _engine->numImages(); i++)
    {
        _descriptorAllocators[i] = createAllocator(i);
    }

    // Cleanup allocators on device destruction
    _engine->cleanupManager().push([this](VkDevice) {
        destroyAllocators();
    });
}

void SMAAProcessor::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createFrameImages(newExtent);
}

const vulkan::Image* SMAAProcessor::process(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    const vulkan::Image* inputImage
)
{
    uint32_t fIdx = frameIndex % _engine->numImages();
    auto& frame = _frames[fIdx];

    SMAAPushConstants pushConstants{};
    pushConstants.texelSize = glm::vec2(1.0f / static_cast<float>(_extent.width), 1.0f / static_cast<float>(_extent.height));

    uint32_t gx = static_cast<uint32_t>(std::ceil(static_cast<float>(_extent.width) / 8.0f));
    uint32_t gy = static_cast<uint32_t>(std::ceil(static_cast<float>(_extent.height) / 8.0f));

    auto& alloc = _descriptorAllocators[fIdx];
    if (!alloc) return frame.outputImage.get();

    // === Pass 1: Edge Detection ===
    {
        vulkan::Image::cmdTransitionImage(cmd, frame.edgesImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        auto ds = alloc->allocate(_edgeDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.edgesImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _edgeDetectionPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _edgePipelineLayout, 0, 1, &dsHandle, 0, nullptr);
        vkCmdPushConstants(cmd, _edgePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pushConstants);
        vkCmdDispatch(cmd, gx, gy, 1);

        vulkan::Image::cmdTransitionImage(cmd, frame.edgesImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // === Pass 2: Blend Weight Generation ===
    {
        vulkan::Image::cmdTransitionImage(cmd, frame.blendWeightsImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        auto ds = alloc->allocate(_blendWeightDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            frame.edgesImage.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _areaTexture.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _searchTexture.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.blendWeightsImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _blendWeightPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _blendPipelineLayout, 0, 1, &dsHandle, 0, nullptr);
        vkCmdPushConstants(cmd, _blendPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pushConstants);
        vkCmdDispatch(cmd, gx, gy, 1);

        vulkan::Image::cmdTransitionImage(cmd, frame.blendWeightsImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // === Pass 3: Neighborhood Blend ===
    {
        vulkan::Image::cmdTransitionImage(cmd, frame.outputImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        auto ds = alloc->allocate(_neighborhoodBlendDSLayout);
        ds->beginUpdate();
        ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            frame.blendWeightsImage.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
        ds->addImage(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            frame.outputImage.get(), VK_IMAGE_LAYOUT_GENERAL);
        ds->endUpdate();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _neighborhoodBlendPipeline);
        VkDescriptorSet dsHandle = ds->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            _nblendPipelineLayout, 0, 1, &dsHandle, 0, nullptr);
        vkCmdPushConstants(cmd, _nblendPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMAAPushConstants), &pushConstants);
        vkCmdDispatch(cmd, gx, gy, 1);

        vulkan::Image::cmdTransitionImage(cmd, frame.outputImage->handle(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return frame.outputImage.get();
}

void SMAAProcessor::cleanup()
{
    cleanupFrameImages();
    destroyAllocators();

    _areaTexture.reset();
    _searchTexture.reset();
}

void SMAAProcessor::cleanupFrameImages()
{
    for (auto& frame : _frames)
    {
        if (frame.edgesImage) frame.edgesImage->cleanup();
        if (frame.blendWeightsImage) frame.blendWeightsImage->cleanup();
        if (frame.outputImage) frame.outputImage->cleanup();
    }
    _frames.clear();
}

void SMAAProcessor::destroyAllocators()
{
    for (auto alloc : _descriptorAllocators)
    {
        if (alloc)
        {
            alloc->destroy();
            delete alloc;
        }
    }
    _descriptorAllocators.clear();
}

}
