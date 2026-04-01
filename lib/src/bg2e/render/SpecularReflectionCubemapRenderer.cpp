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

#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e::render {

SpecularReflectionCubemapRenderer::SpecularReflectionCubemapRenderer(Engine * engine)
    :CubemapRenderer(engine)
{
}
    
void SpecularReflectionCubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    });
}

void SpecularReflectionCubemapRenderer::build(
    std::shared_ptr<vulkan::Image> inputCubemap,
    VkExtent2D cubeImageSize
) {
    CubemapRenderer::build(
        inputCubemap,
        "cubemap_renderer.vert.spv",
        "specular_reflection_renderer.frag.spv",
        cubeImageSize,
        true,
        9,
        VK_FORMAT_R16G16B16A16_SFLOAT
    );
}

void SpecularReflectionCubemapRenderer::createPipelineLayout()
{
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // Projection data buffer
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Input skybox image
    _descriptorSetLayout = dsFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vulkan::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    std::vector<VkDescriptorSetLayout> layouts = {
        _descriptorSetLayout
    };
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.setLayoutCount = uint32_t(layouts.size());
    
    VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &_layout));
    
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipelineLayout(dev, _layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _descriptorSetLayout, nullptr);
    });
}

}
