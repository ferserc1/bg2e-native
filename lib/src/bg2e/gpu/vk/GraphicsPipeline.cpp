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

#include <bg2e/gpu/vk/GraphicsPipeline.hpp>
#include <bg2e/gpu/vk/ShaderModule.hpp>
#include <bg2e/gpu/vk/PipelineLayout.hpp>
#include <bg2e/gpu/vk/VertexDescriptor.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>
#include <bg2e/gpu/Device.hpp>
#include <array>

namespace bg2e {
namespace gpu {
namespace vk {

VkPrimitiveTopology primitiveTopologyToVk(gpu::PrimitiveTopology topology)
{
    switch (topology)
    {
    case gpu::PrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case gpu::PrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case gpu::PrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case gpu::PrimitiveTopology::PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkCullModeFlags cullModeToVk(gpu::CullMode cullMode)
{
    switch (cullMode)
    {
    case gpu::CullMode::None:
        return VK_CULL_MODE_NONE;
    case gpu::CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case gpu::CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_NONE;
}

VkFrontFace frontFaceToVk(gpu::FrontFace frontFace)
{
    switch (frontFace)
    {
    case gpu::FrontFace::Clockwise:
        return VK_FRONT_FACE_CLOCKWISE;
    case gpu::FrontFace::CounterClockwise:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

VkCompareOp compareOperation(CompareOperation value)
{
    const VkCompareOp values[] = {VK_COMPARE_OP_NEVER, VK_COMPARE_OP_LESS, VK_COMPARE_OP_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_GREATER, VK_COMPARE_OP_NOT_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_ALWAYS};
    return values[static_cast<size_t>(value)];
}
VkBlendFactor blendFactor(BlendFactor value)
{
    const VkBlendFactor values[] = {VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA};
    return values[static_cast<size_t>(value)];
}
VkBlendOp blendOperation(BlendOperation value)
{
    const VkBlendOp values[] = {VK_BLEND_OP_ADD, VK_BLEND_OP_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT, VK_BLEND_OP_MIN, VK_BLEND_OP_MAX};
    return values[static_cast<size_t>(value)];
}

GraphicsPipeline::GraphicsPipeline(gpu::Device* gpuDevice, VkDevice device, const gpu::GraphicsPipelineDescription& description)
    : gpu::GraphicsPipeline(gpuDevice), _device(device)
{
    if (!gpuDevice)
    {
        throw std::invalid_argument("Graphics pipeline requires a device");
    }
    validateDescription(description, *gpuDevice);
    setAttachmentDescription(description);

    auto* vkVsModule = dynamic_cast<vk::ShaderModule*>(description.vertexShader);
    auto* vkFsModule = dynamic_cast<vk::ShaderModule*>(description.fragmentShader);
    auto* vkLayout = dynamic_cast<vk::PipelineLayout*>(description.layout);

    if (!vkVsModule || !vkFsModule || !vkLayout)
    {
        throw std::runtime_error("vk::GraphicsPipeline: description contains non-Vulkan objects");
    }

    _layoutHandle = vkLayout->handle();

    // Shader stages
    VkPipelineShaderStageCreateInfo vsStage{};
    vsStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsStage.module = vkVsModule->handle();
    vsStage.pName = vkVsModule->entryPoint().c_str();

    VkPipelineShaderStageCreateInfo fsStage{};
    fsStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsStage.module = vkFsModule->handle();
    fsStage.pName = vkFsModule->entryPoint().c_str();

    VkPipelineShaderStageCreateInfo stages[] = {vsStage, fsStage};

    // Vertex input: built from descriptions (empty = vertex_id only)
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    if (!description.vertexBufferDescriptions.empty())
    {
        vk::VertexDescriptor::buildVkVertexInput(
            description.vertexBufferDescriptions, bindings, attributes);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.empty() ? nullptr : bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = primitiveTopologyToVk(description.topology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport & scissor: dynamic (set at draw time)
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Rasterization: fill, cull mode + front face from description
    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.lineWidth = 1.0f;
    rasterization.cullMode = cullModeToVk(description.cullMode);
    rasterization.frontFace = frontFaceToVk(description.frontFace);
    rasterization.depthBiasEnable = VK_FALSE;

    // Viewport state: required if the rasterization is enabled
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    // Multisample: 1 sample
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkPipelineColorBlendAttachmentState> blends;
    std::vector<VkFormat> colorFormats;
    for (const auto& attachment : _colorAttachments)
    {
        const auto& state = attachment.blend;
        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = state.enabled;
        blend.srcColorBlendFactor = blendFactor(state.sourceColor);
        blend.dstColorBlendFactor = blendFactor(state.destinationColor);
        blend.colorBlendOp = blendOperation(state.colorOperation);
        blend.srcAlphaBlendFactor = blendFactor(state.sourceAlpha);
        blend.dstAlphaBlendFactor = blendFactor(state.destinationAlpha);
        blend.alphaBlendOp = blendOperation(state.alphaOperation);
        blend.colorWriteMask = static_cast<VkColorComponentFlags>(state.writeMask);
        blends.push_back(blend);
        colorFormats.push_back(toVkFormat(attachment.format));
    }
    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = static_cast<uint32_t>(blends.size());
    colorBlend.pAttachments = blends.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    if (description.depthFormat != PixelFormat::Undefined)
    {
        const auto state = description.depthState.value_or(DepthState{});
        depthStencil.depthTestEnable = state.testEnabled || state.writeEnabled;
        depthStencil.depthWriteEnable = state.writeEnabled;
        depthStencil.depthCompareOp = state.testEnabled ? compareOperation(state.compare) : VK_COMPARE_OP_ALWAYS;
    }
    const VkFormat depthFormat = toVkFormat(description.depthFormat);
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
    renderingInfo.pColorAttachmentFormats = colorFormats.data();
    renderingInfo.depthAttachmentFormat = depthFormat;
    renderingInfo.stencilAttachmentFormat = hasStencil(description.depthFormat) ? depthFormat : VK_FORMAT_UNDEFINED;

    // Pipeline create info
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = nullptr; // dynamic
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vkLayout->handle();

    VkResult result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
    if (result != VK_SUCCESS)
    {
        bg2e_log_debug << "vk::GraphicsPipeline: vkCreateGraphicsPipelines failed with result " << result << bg2e_log_end;
        throw std::runtime_error("Failed to create Vulkan graphics pipeline");
    }

    if (base::Log::isDebug() && !description.debugName.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(_pipeline);
        nameInfo.pObjectName = description.debugName.c_str();
        setDebugUtilsObjectName(_device, &nameInfo);
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    cleanup();
}

bool GraphicsPipeline::isValid() const
{
    return _pipeline != VK_NULL_HANDLE;
}

void GraphicsPipeline::cleanup()
{
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
}

}
}
}
