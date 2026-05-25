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
        case DeferredDebugVisualization::GBufferFresnelFlags:
            return gbuffer->image(3).get();
        case DeferredDebugVisualization::GBufferSheenColor:
            return gbuffer->image(4).get();
        case DeferredDebugVisualization::GBufferDepth:
            return gbuffer->depthImage().get();
        case DeferredDebugVisualization::InputImage:
            return inputImage;
        case DeferredDebugVisualization::RTAmbientOcclusion:
            return _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex()).get();
        case DeferredDebugVisualization::DenoisedAO:
            return _denoiseFilter->outputImage(_engine->currentFrameResourcesIndex()).get();
        default:
            return gbuffer->image(0).get();
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

    // Create AO pass
    _rtAmbientOcclusion = std::make_unique<RTAmbientOcclusion>(_engine);
    _rtAmbientOcclusion->build(extent);

    // Create denoise filter
    _denoiseFilter = std::make_unique<DenoiseFilter>(_engine);
    _denoiseFilter->build(_gbuffers[0].get(), extent);

    // Create per-layer data bindings
    _frameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(_engine);
    _fragmentFrameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<scene::vk::EnvironmentDataBinding>(_engine);

    // Detect RT support at build time
    _useRtShadows = _engine->rayTracingSupported();

    // Create G-buffer pipeline
    createGBufferPipeline();

    // Create composite pipeline
    createCompositePipeline();

    // Create composite RT pipeline (only if RT is supported)
    if (_useRtShadows && _rtDataBinding)
    {
        createCompositePipelineRT();
    }

    // Create debug blit pipeline
    createDebugPipeline();

    // Create sampler for G-buffer textures
    vulkan::factory::Sampler samplerFactory(_engine);
    _gbufferSampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _gbufferSampler, nullptr);
        _gbufferSampler = VK_NULL_HANDLE;
    });
}

void DeferredLayer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    _frameDataBinding->initFrameResources(allocator);
    _fragmentFrameDataBinding->initFrameResources(allocator);
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
    auto frameResourcesIndex = _engine->currentFrameResourcesIndex();
    auto* gbuffer = _gbuffers[frameResourcesIndex].get();

    // If this layer is transparent, we need to copy the opaque depth buffer to the transparent depth buffer, because
    // the transparent layer will not modify the depth buffer to preserve the transparency effects
    if (_isTransparent && _opaqueDepthBuffer)
    {
        auto transparentDepthBufferImage = gbuffer->depthImage();
        auto opaqueDepthBufferImage = _opaqueDepthBuffer;
        vulkan::Image::TransitionInfo transitionInfo;
        transitionInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vulkan::Image::cmdTransitionImage(
            cmd,
            transparentDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            transitionInfo
        );
        vulkan::Image::cmdTransitionImage(
            cmd,
            opaqueDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            transitionInfo
        );

        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = {};
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.baseArrayLayer = 0;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = {};
        copyRegion.extent = transparentDepthBufferImage->extent();

        vkCmdCopyImage(
            cmd,
            opaqueDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            transparentDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion
        );

        vulkan::Image::cmdTransitionImage(
            cmd,
            transparentDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            transitionInfo
        );
        vulkan::Image::cmdTransitionImage(
            cmd,
            opaqueDepthBufferImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            transitionInfo
        );
    }

    renderGBufferPass(cmd, currentFrame, gbuffer, frameResources, viewMatrix, projMatrix, cameraWorldPos);

    // AO pass: compute ambient occlusion from G-buffers + TLAS
    {
        auto projMat = _scene->mainCamera()->projectionMatrix();
        auto viewMat = _scene->mainCamera()->viewMatrix();
        auto invVP = glm::inverse(projMat * viewMat);
        _rtAmbientOcclusion->render(cmd, currentFrame, frameResources, gbuffer, invVP);
    }

    // Denoise pass: filter the AO image
    {
        auto aoImg = _rtAmbientOcclusion->aoImage(frameResourcesIndex);
        _denoiseFilter->render(cmd, currentFrame, frameResources, gbuffer, aoImg.get());
    }

    if (_debugVisualization == DeferredDebugVisualization::FullComposition)
    {
        renderCompositePass(cmd, currentFrame, inputImage, outputImage, frameResources, viewMatrix, projMatrix);
    }
    else
    {
        auto* src = resolveDebugSource(inputImage, gbuffer);
        if (src)
        {
            renderDebugPass(cmd, src, outputImage, frameResources);
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
    _rtAmbientOcclusion->resize(newExtent);
    _denoiseFilter->resize(newExtent);
}

void DeferredLayer::cleanup()
{
    for (auto& gb : _gbuffers)
    {
        gb->cleanup();
    }
    _gbuffers.clear();

    if (_rtAmbientOcclusion) _rtAmbientOcclusion->cleanup();
    if (_denoiseFilter) _denoiseFilter->cleanup();

    _frameDataBinding->cleanup();
    _fragmentFrameDataBinding->cleanup();
    _objectDataBinding->cleanup();
    _environmentDataBinding->cleanup();

}

std::shared_ptr<vulkan::Image> DeferredLayer::depthBuffer()
{
    auto gbuffers = _gbuffers[_engine->currentFrameResourcesIndex()].get();
    return gbuffers->depthImage();
}

void DeferredLayer::createGBufferPipeline()
{
    // Create descriptor set layouts
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_frameDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_objectDataBinding->createLayout());

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
    if (_isTransparent)
    {
        plFactory.enableDepthtest(false, VK_COMPARE_OP_LESS);
    }
    else
    {
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    }


    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.disableMultisample();

    // 3 color attachment formats
    plFactory.setColorAttachmentFormat(_gbuffers[0]->formats());

    _gbufferPipeline = plFactory.build(_gbufferPipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _gbufferPipeline, nullptr);
        _gbufferPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _gbufferPipelineLayout, nullptr);
        _gbufferPipelineLayout = VK_NULL_HANDLE;
    });
}

void DeferredLayer::createCompositePipeline()
{
    // Create G-buffer descriptor set layout (6 bindings: 4 G-buffers + 1 input image + 1 depth)
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Albedo
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Normal
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Material
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_FresnelFlags
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_SheenColor
    dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_InputImage
    dsLayoutFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Depth
    dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO
    _compositeGBufferDSLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    // Create pipeline layout
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_compositeGBufferDSLayout);
    layoutFactory.addDescriptorSetLayout(_fragmentFrameDataBinding->createLayout(VK_SHADER_STAGE_FRAGMENT_BIT));
    layoutFactory.addDescriptorSetLayout(_environmentDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout());

    layoutFactory.addPushConstantRange(
        0,
        sizeof(CompositePushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _compositePipelineLayout = layoutFactory.build();

    // Create composite pipeline
    vulkan::factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("deferred_composite.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("deferred_composite.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
        vkDestroyPipeline(dev, _compositePipeline, nullptr);
        vkDestroyPipelineLayout(dev, _compositePipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _compositeGBufferDSLayout, nullptr);
    });
}

void DeferredLayer::createCompositePipelineRT()
{
    if (!_useRtShadows || !_rtDataBinding)
    {
        return;
    }

    // Create pipeline layout with RT data binding
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_compositeGBufferDSLayout);
    layoutFactory.addDescriptorSetLayout(_fragmentFrameDataBinding->createLayout(VK_SHADER_STAGE_FRAGMENT_BIT));
    layoutFactory.addDescriptorSetLayout(_environmentDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_rtDataBinding->createLayout());

    layoutFactory.addPushConstantRange(
        0,
        sizeof(CompositePushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _compositePipelineRTLayout = layoutFactory.build();

    // Create composite RT pipeline
    vulkan::factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("deferred_composite.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("deferred_composite_rt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // No vertex input (fullscreen quad generated procedurally)
    plFactory.clearInputBindingDescriptions();
    plFactory.clearInputAttributeDescriptions();

    plFactory.disableDepthtest();
    plFactory.disableMultisample();
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.setColorAttachmentFormat(_outputFormat);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    _compositePipelineRT = plFactory.build(_compositePipelineRTLayout);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _compositePipelineRT, nullptr);
        _compositePipelineRT = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _compositePipelineRTLayout, nullptr);
        _compositePipelineRTLayout = VK_NULL_HANDLE;
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
        vkDestroyPipeline(dev, _debugPipeline, nullptr);
        vkDestroyPipelineLayout(dev, _debugPipelineLayout, nullptr);
        _debugPipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _debugDSLayout, nullptr);
        _debugDSLayout = VK_NULL_HANDLE;
    });
}

void DeferredLayer::renderGBufferPass(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    GBufferManager * gbuffer,
    vulkan::FrameResources& frameResources,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const glm::vec3& cameraWorldPos
)
{
    gbuffer->beginRender(cmd, _isTransparent);

    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipeline);

    auto sceneDS = _frameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);

    auto dsFunction = [&](MaterialBase* mat, const glm::mat4& transform, uint32_t /*submesh*/) {
        auto objectDS = _objectDataBinding->newDescriptorSet(frameResources, mat, transform);
        return std::vector<VkDescriptorSet> { sceneDS, objectDS };
    };

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

    // Transition G-buffers to shader read
    gbuffer->transitionToShaderRead(cmd);

    // Clear output image and begin rendering
    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    vulkan::macros::cmdClearImageAndBeginRendering(cmd, outputImage, clearValue, VK_IMAGE_LAYOUT_UNDEFINED);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    VkAccelerationStructureKHR tlas = frameResources.rayTracingScene->tlas();

    // Select pipeline based on RT support and whether TLAS is available
    bool useRT = _useRtShadows && tlas != VK_NULL_HANDLE;
    VkPipeline activePipeline = useRT ? _compositePipelineRT : _compositePipeline;
    VkPipelineLayout activeLayout = useRT ? _compositePipelineRTLayout : _compositePipelineLayout;

    // Bind composite pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

    // Create G-buffer descriptor set
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
        gbuffer->image(4).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->addImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    auto denoisedAoImg = _denoiseFilter->outputImage(_engine->currentFrameResourcesIndex());
    gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        denoisedAoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    gbufferDS->endUpdate();

    // Create other descriptor sets
    auto sceneDS = _fragmentFrameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);
    auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment);
    auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lights);


    // Push constants
    auto projMat = _scene->mainCamera()->projectionMatrix();
    auto viewMat = _scene->mainCamera()->viewMatrix();
    auto inverseViewProjection = glm::inverse(projMat * viewMat);
    const CompositePushConstants pc{
        .gamma = 2.2f,
        .brightness = _brightness,
        .contrast = _contrast,
        .exposure = _exposure,
        .numLights = static_cast<uint32_t>(_lights.size()),
        .inverseViewProjection = inverseViewProjection,
    };
    vkCmdPushConstants(
        cmd, activeLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(CompositePushConstants),
        &pc
    );

    // Bind descriptor sets
    VkDescriptorSet gbufferDSPtr[] = { gbufferDS->descriptorSet() };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 0, 1, gbufferDSPtr, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 1, 1, &sceneDS, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 2, 1, &envDS, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        activeLayout, 3, 1, &lightDS, 0, nullptr);

    if (useRT)
    {
        auto rtDS = _rtDataBinding->newDescriptorSet(frameResources, tlas);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            activeLayout, 4, 1, &rtDS,
            0, nullptr
        );
    }

    // Draw fullscreen quad (6 vertices, 2 triangles)
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // End rendering
    vulkan::cmdEndRendering(cmd);
}

void DeferredLayer::renderDebugPass(
    VkCommandBuffer cmd,
    const vulkan::Image* sourceImage,
    const vulkan::Image* outputImage,
    vulkan::FrameResources& frameResources
)
{

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
