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

#include <bg2e/render/GPUProcess.hpp>
#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>

namespace bg2e::render {

GPUProcess::GPUProcess(Engine * engine)
    :_engine{engine}
{
}

void GPUProcess::addBinding(
    uint32_t binding,
    vulkan::Image * img,
    VkDescriptorType type,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout
) {
    _bindings.push_back({
        binding,
        { .image = img },
        BindingType::TypeImage,
        type,
        initialLayout,
        finalLayout,
        0,
        0
    });
}

void GPUProcess::addBinding(
    uint32_t binding,
    vulkan::Buffer * buffer,
    VkDescriptorType type,
    uint32_t bufferSize,
    uint32_t bufferOffset
) {
    _bindings.push_back({
        .index = binding,
        .data = { .buffer = buffer },
        .type = BindingType::TypeBuffer,
        .descriptorType = type,
        .bufferSize = bufferSize,
        .bufferOffset = bufferOffset
    });
}

void GPUProcess::executeShader(const std::string& shaderFile, VkExtent2D imageExtent)
{
    vulkan::factory::ComputePipeline plFactory(_engine);
    vulkan::factory::DescriptorSetLayout dsFactory;
    vulkan::factory::PipelineLayout loFactory(_engine);
    
    auto dsAllocator = new vulkan::DescriptorSetAllocator();
    dsAllocator->init(_engine);
    
    VkPipelineLayout layout;
    VkPipeline pipeline;
    
    for (auto binding : _bindings)
    {
        if (binding.type == TypeImage)
        {
            dsFactory.addBinding(binding.index, binding.descriptorType);
            dsAllocator->requirePoolSizeRatio(1, {
                { binding.descriptorType, 1 }
            });
        }
        else if (binding.type == TypeBuffer)
        {
            dsFactory.addBinding(binding.index, binding.descriptorType);
            dsAllocator->requirePoolSizeRatio(1, {
                { binding.descriptorType, 1 }
            });
        }
    }
    dsAllocator->initPool();
    
    auto descriptorSetLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);
    
    loFactory.addDescriptorSetLayout(descriptorSetLayout);
    layout = loFactory.build("GPUProcess::PipelineLayout");
    
    plFactory.setShader(shaderFile);
    pipeline = plFactory.build(layout, "GPUProcess::Pipeline");

    _engine->command().immediateSubmit([&, pipeline, layout, descriptorSetLayout, dsAllocator](VkCommandBuffer cmd) {
        auto descriptorSet = dsAllocator->allocate(descriptorSetLayout);
        descriptorSet->beginUpdate();
        for (auto binding : _bindings)
        {
            if (binding.type == TypeImage)
            {
                descriptorSet->addImage(
                    binding.index, binding.descriptorType,
                    binding.data.image, binding.initialLayout
                );
            }
            else if (binding.type == TypeBuffer)
            {
                descriptorSet->addBuffer(
                    binding.index, binding.descriptorType,
                    binding.data.buffer, binding.bufferSize,
                    binding.bufferOffset
                );
            }
        }
        descriptorSet->endUpdate();
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        
        VkDescriptorSet ds = descriptorSet->descriptorSet();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            layout,
            0,
            1,
            &ds,
            0,
            nullptr
        );
        
        vkCmdDispatch(
            cmd,
            static_cast<uint32_t>(std::ceil(imageExtent.width / 16)),
            static_cast<uint32_t>(std::ceil(imageExtent.height / 16)),
            1
        );
        
        for (auto binding : _bindings)
        {
            if (binding.type == TypeImage)
            {
                if (binding.initialLayout != binding.finalLayout)
                {
                    vulkan::Image::cmdTransitionImage(
                        cmd,
                        binding.data.image->handle(),
                        binding.initialLayout,
                        binding.finalLayout
                    );
                }
            }
        }
    });
    
    delete dsAllocator;
    
    vkDestroyPipeline(_engine->device().handle(), pipeline, nullptr);
    vkDestroyPipelineLayout(_engine->device().handle(), layout, nullptr);
    vkDestroyDescriptorSetLayout(_engine->device().handle(), descriptorSetLayout, nullptr);
}

}
