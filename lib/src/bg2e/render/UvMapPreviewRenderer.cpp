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

#include <bg2e/render/UvMapPreviewRenderer.hpp>

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace bg2e::render {

namespace {

// Non-indexed vertex: UV position, barycentric coordinates for the
// in-shader wireframe, and the submesh index used for tinting.
struct PreviewVertex
{
    glm::vec2 uv;
    glm::vec3 barycentric;
    uint32_t submesh;
};

struct PreviewPushConstants
{
    float lineWidth;
    uint32_t mode; // 0 = UV triangles, 1 = atlas boundary lines
};

constexpr uint32_t kTriangleMode = 0;
constexpr uint32_t kBoundaryMode = 1;

VkVertexInputBindingDescription previewBinding()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(PreviewVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

std::vector<VkVertexInputAttributeDescription> previewAttributes()
{
    std::vector<VkVertexInputAttributeDescription> attributes(3);
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[0].offset = offsetof(PreviewVertex, uv);
    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = offsetof(PreviewVertex, barycentric);
    attributes[2].location = 2;
    attributes[2].binding = 0;
    attributes[2].format = VK_FORMAT_R32_UINT;
    attributes[2].offset = offsetof(PreviewVertex, submesh);
    return attributes;
}

} // anonymous namespace

UvMapPreviewRenderer::UvMapPreviewRenderer(Engine * engine, uint32_t resolution)
    : _engine(engine)
    , _resolution(resolution)
{
    if (!_engine)
    {
        throw std::invalid_argument("UvMapPreviewRenderer: engine must not be null");
    }
    if (_resolution == 0)
    {
        throw std::invalid_argument("UvMapPreviewRenderer: resolution must be greater than zero");
    }
    createTarget();
    createResources();
}

UvMapPreviewRenderer::~UvMapPreviewRenderer()
{
    destroyPipelines();
}

void UvMapPreviewRenderer::setResolution(uint32_t resolution)
{
    if (resolution == 0)
    {
        throw std::invalid_argument("UvMapPreviewRenderer: resolution must be greater than zero");
    }
    if (resolution == _resolution)
    {
        return;
    }
    _engine->device().waitIdle();
    _resolution = resolution;
    createTarget();
}

void UvMapPreviewRenderer::createTarget()
{
    auto * image = vulkan::Image::createAllocatedImage(
        _engine,
        "UvMapPreviewRenderer target",
        VK_FORMAT_R8G8B8A8_UNORM,
        { _resolution, _resolution },
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    );

    auto * baseTexture = new base::Texture();
    baseTexture->setUseMipmaps(false);
    baseTexture->setMagFilter(base::Texture::FilterLinear);
    baseTexture->setMinFilter(base::Texture::FilterLinear);

    auto texture = std::make_unique<Texture>(_engine);
    texture->load(baseTexture, image);
    _texture = std::move(texture);
    _targetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void UvMapPreviewRenderer::createResources()
{
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(PreviewPushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);
    _pipelineLayout = layoutFactory.build("UvMapPreviewRenderer::PipelineLayout");

    vulkan::factory::GraphicsPipeline pipelineFactory(_engine);
    pipelineFactory.setInputBindingDescription(previewBinding());
    pipelineFactory.setInputAttributeDescriptions(previewAttributes());
    pipelineFactory.disableMultisample();
    pipelineFactory.disableDepthtest();
    pipelineFactory.disableBlending();
    pipelineFactory.setColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM);
    pipelineFactory.setDepthFormat(VK_FORMAT_UNDEFINED);
    pipelineFactory.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineFactory.addShader("uv_map_preview.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("uv_map_preview.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    _trianglePipeline = pipelineFactory.build(_pipelineLayout, "UvMapPreviewRenderer::TrianglePipeline");

    pipelineFactory.clearShaders();
    pipelineFactory.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    pipelineFactory.addShader("uv_map_preview.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("uv_map_preview.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    _linePipeline = pipelineFactory.build(_pipelineLayout, "UvMapPreviewRenderer::BoundaryPipeline");

    // [0,1] atlas boundary as a line list
    const PreviewVertex border[] = {
        { { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 }, { { 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 },
        { { 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 }, { { 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0 },
        { { 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0 }, { { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0 },
        { { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0 }, { { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0 }
    };
    _borderBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _engine,
        sizeof(border),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        "UvMapPreviewRenderer atlas boundary"
    ));
    std::memcpy(_borderBuffer->allocatedData(), border, sizeof(border));
    _borderBuffer->flushAllocatedData();
}

void UvMapPreviewRenderer::destroyPipelines()
{
    if (_trianglePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _trianglePipeline, nullptr);
        _trianglePipeline = VK_NULL_HANDLE;
    }
    if (_linePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _linePipeline, nullptr);
        _linePipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
}

void UvMapPreviewRenderer::render(const geo::Mesh & mesh, uint32_t uvSet)
{
    if (uvSet > 1)
    {
        throw std::invalid_argument("UvMapPreviewRenderer: uvSet must be 0 (UV1) or 1 (UV2)");
    }

    // Expand the selected UV channel into a non-indexed triangle list with
    // barycentric coordinates. Invalid indices are skipped: the preview is a
    // diagnostic view and must tolerate meshes that fail UvAtlasValidator.
    std::vector<PreviewVertex> vertices;
    const auto appendTriangle = [&](uint32_t i0, uint32_t i1, uint32_t i2, uint32_t submeshIndex) {
        const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
        {
            return;
        }
        const auto uvOf = [uvSet](const geo::VertexPNUUT & vertex) {
            return uvSet == 0 ? vertex.texCoord0 : vertex.texCoord1;
        };
        vertices.push_back({ uvOf(mesh.vertices[i0]), { 1.0f, 0.0f, 0.0f }, submeshIndex });
        vertices.push_back({ uvOf(mesh.vertices[i1]), { 0.0f, 1.0f, 0.0f }, submeshIndex });
        vertices.push_back({ uvOf(mesh.vertices[i2]), { 0.0f, 0.0f, 1.0f }, submeshIndex });
    };

    if (!mesh.submeshes.empty())
    {
        for (uint32_t submeshIndex = 0; submeshIndex < mesh.submeshes.size(); ++submeshIndex)
        {
            const auto & submesh = mesh.submeshes[submeshIndex];
            const uint64_t end = std::min<uint64_t>(
                static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount,
                mesh.indices.size());
            for (uint64_t index = submesh.firstIndex; index + 2 < end; index += 3)
            {
                appendTriangle(mesh.indices[index], mesh.indices[index + 1],
                    mesh.indices[index + 2], submeshIndex);
            }
        }
    }
    else
    {
        for (size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
        {
            appendTriangle(mesh.indices[index], mesh.indices[index + 1],
                mesh.indices[index + 2], 0);
        }
    }

    // Host-visible upload: render() blocks until the submission completes, so
    // the buffer can be released as soon as immediateSubmit returns.
    std::unique_ptr<vulkan::Buffer> vertexBuffer;
    const VkDeviceSize vertexBytes = vertices.size() * sizeof(PreviewVertex);
    if (vertexBytes > 0)
    {
        vertexBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
            _engine,
            vertexBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            "UvMapPreviewRenderer vertices"
        ));
        std::memcpy(vertexBuffer->allocatedData(), vertices.data(), vertexBytes);
        vertexBuffer->flushAllocatedData();
    }

    auto * targetImage = _texture->image();
    const VkExtent2D extent { _resolution, _resolution };
    const PreviewPushConstants triangleParams { 1.5f / static_cast<float>(_resolution), kTriangleMode };
    const PreviewPushConstants boundaryParams { 0.0f, kBoundaryMode };
    const auto vertexCount = static_cast<uint32_t>(vertices.size());
    const auto initialLayout = _targetLayout;
    const VkBuffer vertexHandle = vertexBuffer ? vertexBuffer->handle() : VK_NULL_HANDLE;
    const VkBuffer borderHandle = _borderBuffer->handle();

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        VkClearColorValue clearValue { { 0.0f, 0.0f, 0.0f, 1.0f } };
        vulkan::macros::cmdClearImageAndBeginRendering(cmd, targetImage, clearValue, initialLayout);
        vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, extent);

        if (vertexHandle != VK_NULL_HANDLE)
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
            vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(PreviewPushConstants), &triangleParams);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vertexHandle, &offset);
            vkCmdDraw(cmd, vertexCount, 1, 0, 0);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _linePipeline);
        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PreviewPushConstants), &boundaryParams);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &borderHandle, &offset);
        vkCmdDraw(cmd, 8, 1, 0, 0);

        vulkan::cmdEndRendering(cmd);

        vulkan::Image::TransitionInfo toSampling(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
        );
        vulkan::Image::cmdTransitionImage(cmd, targetImage->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, toSampling);
    });

    _targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

} // namespace bg2e::render
