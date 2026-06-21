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

#include <bg2e/render/deferred/MotionVectorGenerator.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <cmath>

namespace bg2e::render::deferred {

MotionVectorGenerator::MotionVectorGenerator(Engine* engine)
    : _engine{engine}
{
}

MotionVectorGenerator::~MotionVectorGenerator()
{
    cleanup();
}

void MotionVectorGenerator::build(VkExtent2D renderExtent)
{
    _extent = renderExtent;

    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build(
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    );
    _engine->cleanupManager().push([this](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    createPipeline();
    createDescriptorAllocators();
    createFrameImages(renderExtent);
}

void MotionVectorGenerator::createPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(0, sizeof(PushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    _pipelineLayout = layoutFactory.build("MotionVectorGenerator::PipelineLayout");

    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader("motion_vectors.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout, "MotionVectorGenerator::Pipeline");

    _engine->cleanupManager().push([this](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}

void MotionVectorGenerator::createFrameImages(VkExtent2D extent)
{
    cleanupFrameImages();

    _frames.resize(_engine->numImages());
    for (uint32_t i = 0; i < _frames.size(); i++)
    {
        _frames[i].motionVectors = std::unique_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "MotionVectors image " + std::to_string(i),
                VK_FORMAT_R16G16_SFLOAT,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}

void MotionVectorGenerator::cleanupFrameImages()
{
    for (auto& f : _frames)
    {
        if (f.motionVectors) f.motionVectors->cleanup();
    }
    _frames.clear();
}

void MotionVectorGenerator::createDescriptorAllocators()
{
    destroyAllocators();

    _descriptorAllocators.resize(_engine->numImages(), nullptr);
    for (uint32_t i = 0; i < _engine->numImages(); i++)
    {
        auto* alloc = new vulkan::DescriptorSetAllocator();
        alloc->init(_engine);
        alloc->requirePoolSizeRatio(4, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.5f },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          0.5f }
        });
        alloc->initPool();
        _descriptorAllocators[i] = alloc;
    }

    _engine->cleanupManager().push([this](VkDevice) {
        destroyAllocators();
    });
}

void MotionVectorGenerator::destroyAllocators()
{
    for (auto* alloc : _descriptorAllocators)
    {
        if (alloc) { alloc->destroy(); delete alloc; }
    }
    _descriptorAllocators.clear();
}

void MotionVectorGenerator::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createFrameImages(newExtent);
}

const vulkan::Image* MotionVectorGenerator::generate(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    const vulkan::Image* depthImage,
    const glm::mat4& currentInvViewProj,
    const glm::mat4& prevViewProj
)
{
    uint32_t fIdx = frameIndex % _engine->numImages();
    auto& frame = _frames[fIdx];
    auto* alloc = _descriptorAllocators[fIdx];

    // Depth is already in SHADER_READ_ONLY_OPTIMAL after the composite pass
    vulkan::Image::cmdTransitionImage(
        cmd, frame.motionVectors->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );

    auto ds = alloc->allocate(_dsLayout);
    ds->beginUpdate();
    // depthImage was created with VK_IMAGE_ASPECT_DEPTH_BIT, so imageView() is correct
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        depthImage->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        frame.motionVectors.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->endUpdate();

    PushConstants pc;
    pc.currentInvViewProj = currentInvViewProj;
    pc.prevViewProj       = prevViewProj;
    pc.renderSize         = glm::vec2(static_cast<float>(_extent.width),
                                      static_cast<float>(_extent.height));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstants), &pc);

    uint32_t gx = static_cast<uint32_t>(std::ceil(static_cast<float>(_extent.width)  / 8.0f));
    uint32_t gy = static_cast<uint32_t>(std::ceil(static_cast<float>(_extent.height) / 8.0f));
    vkCmdDispatch(cmd, gx, gy, 1);

    // Leave depth in SHADER_READ_ONLY_OPTIMAL for FSR to read it next
    // Leave motion vectors in GENERAL (storage image state) — FSR will transition as needed

    return frame.motionVectors.get();
}

void MotionVectorGenerator::cleanup()
{
    cleanupFrameImages();
    destroyAllocators();
}

}
