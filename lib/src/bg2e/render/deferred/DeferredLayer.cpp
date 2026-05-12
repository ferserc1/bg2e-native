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

#include <bg2e/render/deferred/DeferredLayer.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

namespace bg2e::render::deferred {

DeferredLayer::DeferredLayer(Engine* engine, LayerType type)
    : RenderLayer { engine }
    , _layerType { type }
{
}

DeferredLayer::~DeferredLayer()
{
    cleanup();
}

const vulkan::Image* DeferredLayer::resolveDebugSource(const vulkan::Image* inputImage, GBufferManager* gbuffer) const
{
    switch (_debugVisualization)
    {
        case DeferredDebugVisualization::GBufferAlbedo:
            return gbuffer->image(0).get();
        case DeferredDebugVisualization::GBufferNormal:
            return gbuffer->image(1).get();
        case DeferredDebugVisualization::GBufferMaterial:
            return gbuffer->image(2).get();
        case DeferredDebugVisualization::GBufferPosition:
            return gbuffer->image(3).get();
        case DeferredDebugVisualization::GBufferDepth:
            return gbuffer->depthImage().get();
        case DeferredDebugVisualization::InputImage:
            return inputImage;
        default:
            return nullptr;
    }
}

void DeferredLayer::build(VkExtent2D extent, VkFormat outputFormat)
{
    RenderLayer::build(extent, outputFormat);

    // Create per-frame G-buffer managers
    _gbuffers.resize(_engine->numImages());
    for (auto& gb : _gbuffers)
    {
        gb = std::make_unique<GBufferManager>(_engine);
        gb->build(extent);
    }

    // Create per-layer data bindings
    _frameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<scene::vk::EnvironmentDataBinding>(_engine);

    // Detect RT support at build time
    _useRtShadows = _engine->rayTracingSupported();

    // Create G-buffer pipeline
    createGBufferPipeline();

    // Create composite pipeline
    // TODO: Debug. Create the composite pipeline again after debug g-buffers
    //createCompositePipeline();

    // Create debug blit pipeline
    createDebugPipeline();

    // Create sampler for G-buffer textures
    vulkan::factory::Sampler samplerFactory(_engine);
    _gbufferSampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice) {
        _gbufferSampler = VK_NULL_HANDLE;
    });
}

void DeferredLayer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    _frameDataBinding->initFrameResources(allocator);
    _objectDataBinding->initFrameResources(allocator);
    _environmentDataBinding->initFrameResources(allocator);
}

void DeferredLayer::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vulkan::Image* inputImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources
)
{
    if (!_scene || !_renderQueue)
    {
        return;
    }

    auto mainCamera = _scene->mainCamera();
    if (!mainCamera)
    {
        return;
    }

    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
    auto projMatrix = mainCamera->projectionMatrix();
    auto cameraWorldPos = mainCamera->ownerNode()->worldPosition();

    renderGBufferPass(cmd, currentFrame, frameResources, viewMatrix, projMatrix, cameraWorldPos);

    auto frameIndex = _engine->currentFrameResourcesIndex();
    auto* gbuffer = _gbuffers[frameIndex].get();

    if (_debugVisualization == DeferredDebugVisualization::FullComposition)
    {
        renderCompositePass(cmd, currentFrame, inputImage, outputImage, frameResources, viewMatrix, projMatrix);
    }
    else
    {
        auto* src = resolveDebugSource(inputImage, gbuffer);
        if (src)
        {
            //renderDebugPass(cmd, src, outputImage, frameResources);
        }
    }
}

void DeferredLayer::resize(VkExtent2D newExtent)
{
    RenderLayer::resize(newExtent);

    for (auto& gb : _gbuffers)
    {
        gb->resize(newExtent);
    }

    // Recreate pipelines with new extent
    cleanup();
    createGBufferPipeline();
    createCompositePipeline();
    createDebugPipeline();
}

void DeferredLayer::cleanup()
{
    _gbuffers.clear();

    if (_gbufferPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _gbufferPipeline, nullptr);
        _gbufferPipeline = VK_NULL_HANDLE;
    }
    if (_compositePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _compositePipeline, nullptr);
        _compositePipeline = VK_NULL_HANDLE;
    }
    if (_gbufferPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _gbufferPipelineLayout, nullptr);
        _gbufferPipelineLayout = VK_NULL_HANDLE;
    }
    if (_compositePipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _compositePipelineLayout, nullptr);
        _compositePipelineLayout = VK_NULL_HANDLE;
    }
    if (_gbufferFrameDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _gbufferFrameDSLayout, nullptr);
        _gbufferFrameDSLayout = VK_NULL_HANDLE;
    }
    if (_gbufferObjectDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _gbufferObjectDSLayout, nullptr);
        _gbufferObjectDSLayout = VK_NULL_HANDLE;
    }
    if (_gbufferEnvDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _gbufferEnvDSLayout, nullptr);
        _gbufferEnvDSLayout = VK_NULL_HANDLE;
    }
    if (_gbufferLightDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _gbufferLightDSLayout, nullptr);
        _gbufferLightDSLayout = VK_NULL_HANDLE;
    }
    if (_gbufferRtDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _gbufferRtDSLayout, nullptr);
        _gbufferRtDSLayout = VK_NULL_HANDLE;
    }
    if (_compositeGBufferDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _compositeGBufferDSLayout, nullptr);
        _compositeGBufferDSLayout = VK_NULL_HANDLE;
    }
    if (_debugPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _debugPipeline, nullptr);
        _debugPipeline = VK_NULL_HANDLE;
    }
    if (_debugPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _debugPipelineLayout, nullptr);
        _debugPipelineLayout = VK_NULL_HANDLE;
    }
    if (_debugDSLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _debugDSLayout, nullptr);
        _debugDSLayout = VK_NULL_HANDLE;
    }
}

void DeferredLayer::createGBufferPipeline()
{
    // Create descriptor set layouts
    _gbufferFrameDSLayout = _frameDataBinding->createLayout();
    _gbufferObjectDSLayout = _objectDataBinding->createLayout();
    _gbufferEnvDSLayout = _environmentDataBinding->createLayout();
    _gbufferLightDSLayout = _lightDataBinding->createLayout();

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_gbufferFrameDSLayout);
    layoutFactory.addDescriptorSetLayout(_gbufferObjectDSLayout);
    layoutFactory.addDescriptorSetLayout(_gbufferEnvDSLayout);
    layoutFactory.addDescriptorSetLayout(_gbufferLightDSLayout);

    if (_useRtShadows && _rtDataBinding)
    {
        _gbufferRtDSLayout = _rtDataBinding->createLayout();
        layoutFactory.addDescriptorSetLayout(_gbufferRtDSLayout);
    }

    layoutFactory.addPushConstantRange(
        0,
        sizeof(CompositePushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _gbufferPipelineLayout = layoutFactory.build();

    // Create G-buffer pipeline
    vulkan::factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("deferred_gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("deferred_gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    plFactory.setInputState<scene::Drawable>();

    plFactory.setDepthFormat(_gbuffers[0]->depthFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.disableMultisample();

    // 4 color attachment formats
    plFactory.setColorAttachmentFormat(_gbuffers[0]->formats());

    _gbufferPipeline = plFactory.build(_gbufferPipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        if (_gbufferPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(dev, _gbufferPipeline, nullptr);
            _gbufferPipeline = VK_NULL_HANDLE;
        }
        if (_gbufferPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(dev, _gbufferPipelineLayout, nullptr);
            _gbufferPipelineLayout = VK_NULL_HANDLE;
        }
        if (_gbufferFrameDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _gbufferFrameDSLayout, nullptr);
            _gbufferFrameDSLayout = VK_NULL_HANDLE;
        }
        if (_gbufferObjectDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _gbufferObjectDSLayout, nullptr);
            _gbufferObjectDSLayout = VK_NULL_HANDLE;
        }
        if (_gbufferEnvDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _gbufferEnvDSLayout, nullptr);
            _gbufferEnvDSLayout = VK_NULL_HANDLE;
        }
        if (_gbufferLightDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _gbufferLightDSLayout, nullptr);
            _gbufferLightDSLayout = VK_NULL_HANDLE;
        }
        if (_gbufferRtDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _gbufferRtDSLayout, nullptr);
            _gbufferRtDSLayout = VK_NULL_HANDLE;
        }
    });
}

void DeferredLayer::createCompositePipeline()
{
    // Create G-buffer descriptor set layout (5 bindings: 4 G-buffers + 1 input image)
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Albedo
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Normal
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Material
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Position
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_InputImage
    _compositeGBufferDSLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    // Create pipeline layout
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_compositeGBufferDSLayout);
    layoutFactory.addDescriptorSetLayout(_frameDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_environmentDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout());

    if (_useRtShadows && _rtDataBinding)
    {
        layoutFactory.addDescriptorSetLayout(_rtDataBinding->createLayout());
    }

    layoutFactory.addPushConstantRange(
        0,
        sizeof(CompositePushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _compositePipelineLayout = layoutFactory.build();

    // Create composite pipeline
    vulkan::factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("deferred_composite.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    if (_useRtShadows)
    {
        plFactory.addShader("deferred_composite_rt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else
    {
        plFactory.addShader("deferred_composite.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // No vertex input (fullscreen quad generated procedurally)
    plFactory.clearInputBindingDescriptions();
    plFactory.clearInputAttributeDescriptions();

    plFactory.disableDepthtest();
    plFactory.disableMultisample();
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.setColorAttachmentFormat(_outputFormat);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    _compositePipeline = plFactory.build(_compositePipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        if (_compositePipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(dev, _compositePipeline, nullptr);
            _compositePipeline = VK_NULL_HANDLE;
        }
        if (_compositePipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(dev, _compositePipelineLayout, nullptr);
            _compositePipelineLayout = VK_NULL_HANDLE;
        }
        if (_compositeGBufferDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _compositeGBufferDSLayout, nullptr);
            _compositeGBufferDSLayout = VK_NULL_HANDLE;
        }
    });
}

void DeferredLayer::createDebugPipeline()
{
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _debugDSLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_debugDSLayout);
    _debugPipelineLayout = layoutFactory.build();

    vulkan::factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("deferred_debug_blit.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("deferred_debug_blit.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    plFactory.clearInputBindingDescriptions();
    plFactory.clearInputAttributeDescriptions();

    plFactory.disableDepthtest();
    plFactory.disableMultisample();
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.setColorAttachmentFormat(_outputFormat);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    _debugPipeline = plFactory.build(_debugPipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        if (_debugPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(dev, _debugPipeline, nullptr);
            _debugPipeline = VK_NULL_HANDLE;
        }
        if (_debugPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(dev, _debugPipelineLayout, nullptr);
            _debugPipelineLayout = VK_NULL_HANDLE;
        }
        if (_debugDSLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(dev, _debugDSLayout, nullptr);
            _debugDSLayout = VK_NULL_HANDLE;
        }
    });
}

void DeferredLayer::renderGBufferPass(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const glm::vec3& cameraWorldPos
)
{

    auto* gbuffer = _gbuffers[_engine->currentFrameResourcesIndex()].get();

    /*
    // 1. Clear G-buffers
    gbuffer->clear(cmd);

    // 2. Transition G-buffers to attachment layout
    gbuffer->transitionToAttachment(cmd);

    // 3. Begin dynamic rendering with 4 color attachments + depth
    vulkan::macros::cmdClearImagesAndBeginRendering(
        cmd,
        gbuffer->images(),
        { {0, 0, 0, 0} },
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        gbuffer->depthImage().get(),
        1.0f
    );
*/
    gbuffer->beginRender(cmd);

    // 4. Set viewport/scissor
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    // 5. Bind G-buffer pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipeline);

    // 6. Create descriptor sets
    auto sceneDS = _frameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);
    auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment);
    auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

    // 7. Create descriptor set function
    auto dsFunction = [&](MaterialBase* mat, const glm::mat4& transform, uint32_t /*submesh*/) {
        auto objectDS = _objectDataBinding->newDescriptorSet(frameResources, mat, transform);
        if (_useRtShadows && _rtDataBinding)
        {
            auto tlas = frameResources.rayTracingScene->tlas();
            auto rtDS = _rtDataBinding->newDescriptorSet(frameResources, tlas);
            return std::vector<VkDescriptorSet> { sceneDS, objectDS, envDS, lightDS, rtDS };
        }
        return std::vector<VkDescriptorSet> { sceneDS, objectDS, envDS, lightDS };
    };

    // 8. Render queue items based on layer type
    if (_layerType == LayerType::Opaque)
    {
        _renderQueue->render(
            RenderQueueType::Opaque,
            cmd,
            _gbufferPipelineLayout,
            dsFunction,
            cameraWorldPos
        );
    }
    else
    {
        _renderQueue->render(
            RenderQueueType::Transparent,
            cmd,
            _gbufferPipelineLayout,
            dsFunction,
            cameraWorldPos
        );
        _renderQueue->render(
            RenderQueueType::SolidTransparent,
            cmd,
            _gbufferPipelineLayout,
            dsFunction,
            cameraWorldPos
        );
    }

    // 9. End rendering
    vulkan::cmdEndRendering(cmd);

    gbuffer->transitionToShaderRead(cmd);
}

void DeferredLayer::renderCompositePass(
    VkCommandBuffer cmd,
    uint32_t /*currentFrame*/,
    const vulkan::Image* inputImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix
)
{
    auto* gbuffer = _gbuffers[_engine->currentFrameResourcesIndex()].get();

    // 1. Transition G-buffers to shader read
    gbuffer->transitionToShaderRead(cmd);

    // 2. Clear output image and begin rendering
    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, outputImage, clearValue, VK_IMAGE_LAYOUT_UNDEFINED);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    // 3. Bind composite pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _compositePipeline);

    // 4. Create G-buffer descriptor set
    auto gbufferDS = frameResources.newDescriptorSet(_compositeGBufferDSLayout);
    gbufferDS->beginUpdate();
    gbufferDS->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(0).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(2).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(3).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->endUpdate();

    // 5. Create other descriptor sets
    auto sceneDS = _frameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);
    auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment);
    auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

    // 6. Bind descriptor sets
    VkDescriptorSet gbufferDSPtr[] = { gbufferDS->descriptorSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _compositePipelineLayout, 0, 1, gbufferDSPtr, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _compositePipelineLayout, 1, 1, &sceneDS, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _compositePipelineLayout, 2, 1, &envDS, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _compositePipelineLayout, 3, 1, &lightDS, 0, nullptr);

    if (_useRtShadows && _rtDataBinding)
    {
        auto tlas = frameResources.rayTracingScene->tlas();
        auto rtDS = _rtDataBinding->newDescriptorSet(frameResources, tlas);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            _compositePipelineLayout, 4, 1, &rtDS, 0, nullptr);
    }

    // 7. Push constants
    CompositePushConstants pc{ 2.2f, _brightness, _contrast, _exposure };
    vkCmdPushConstants(cmd, _compositePipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants), &pc);

    // 8. Draw fullscreen quad (6 vertices, 2 triangles)
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // 9. End rendering
    vulkan::cmdEndRendering(cmd);
}

void DeferredLayer::renderDebugPass(
    VkCommandBuffer cmd,
    const vulkan::Image* sourceImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources
)
{
    auto* gbuffer = _gbuffers[_engine->currentFrameResourcesIndex()].get();

    // 1. Transition G-buffers to shader read (same as composite pass)
    gbuffer->transitionToShaderRead(cmd);

    // 2. Ensure source image is in shader read layout
    vulkan::Image::cmdTransitionImage(cmd, sourceImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // 3. Clear output image and begin rendering
    VkClearColorValue clearValue{ { 0.0f, 0.0f, 1.0f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, outputImage, clearValue, VK_IMAGE_LAYOUT_UNDEFINED);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    // 4. Bind debug pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _debugPipeline);

    // 5. Create and bind descriptor set with source image
    auto ds = frameResources.newDescriptorSet(_debugDSLayout);
    ds->beginUpdate();
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        sourceImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    ds->endUpdate();

    VkDescriptorSet dsPtr[] = { ds->descriptorSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _debugPipelineLayout, 0, 1, dsPtr, 0, nullptr);

    // 6. Draw fullscreen quad
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // 7. End rendering
    vulkan::cmdEndRendering(cmd);
}

}
