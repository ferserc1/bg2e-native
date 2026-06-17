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
        case DeferredDebugVisualization::TemporalAccumulatedAO:
            return _temporalAccumulator
                ? _temporalAccumulator->outputImage(_engine->currentFrameResourcesIndex()).get()
                : _rtAmbientOcclusion->aoImage(_engine->currentFrameResourcesIndex()).get();
        case DeferredDebugVisualization::RTReflections:
            return _rtReflections ? _rtReflections->reflectionImage(_engine->currentFrameResourcesIndex()).get() : nullptr;
        case DeferredDebugVisualization::TemporalAccumulatedReflections:
            return _temporalReflectionAccumulator
                ? _temporalReflectionAccumulator->outputImage(_engine->currentFrameResourcesIndex()).get()
                : nullptr;
        case DeferredDebugVisualization::RTReflectionMask:
            return _temporalReflectionAccumulator
                ? _temporalReflectionAccumulator->outputImage(_engine->currentFrameResourcesIndex()).get()
                : nullptr;
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

    // Create temporal accumulator (only if RT is supported)
    if (_engine->rayTracingSupported())
    {
        _temporalAccumulator = std::make_unique<TemporalAccumulator>(_engine);
        _temporalAccumulator->build(_gbuffers[0].get(), extent);
    }

    // Create denoise filter
    _denoiseFilter = std::make_unique<DenoiseFilter>(_engine);
    _denoiseFilter->build(_gbuffers[0].get(), extent);

    // Create RT reflections subsystem (only if RT is supported)
    if (_engine->rayTracingSupported())
    {
        _rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(_engine);

        _rtReflections = std::make_unique<RTReflections>(_engine);
        _rtReflections->setMaterialDataBinding(_rtMaterialDataBinding.get());
        _rtReflections->setReflectionLightDataBinding(_reflectionLightDataBinding);
        _rtReflections->build(_gbuffers[0].get(), extent);

        _temporalReflectionAccumulator = std::make_unique<TemporalAccumulator>(_engine);
        _temporalReflectionAccumulator->setFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
        _temporalReflectionAccumulator->setIsHDR(true);
        _temporalReflectionAccumulator->build(_gbuffers[0].get(), extent);
    }

    // Create fallback image for RT reflections (1x1, alpha=0 → cubemap fallback)
    {
        std::vector<uint8_t> blackData(4 * 4 * sizeof(uint16_t), 0);
        _rtReflectionFallbackImage = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine, "DeferredLayer: RT Reflections fallback", blackData.data(),
                VkExtent2D{1, 1}, 4, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            )
        );
    }

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
    if (_rtMaterialDataBinding)
    {
        _rtMaterialDataBinding->initFrameResources(allocator);
    }
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

    auto useRT = _engine->rayTracingSupported();
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

    const vulkan::Image* reflectionInputForComposite = nullptr;

    // AO pass: compute ambient occlusion from G-buffers + TLAS
    if (useRT)
    {
        auto projMat = _scene->mainCamera()->projectionMatrix();
        auto viewMat = _scene->mainCamera()->viewMatrix();
        auto invVP = glm::inverse(projMat * viewMat);
        _rtAmbientOcclusion->render(cmd, currentFrame, frameResources, gbuffer, invVP);

        // Temporal accumulation pass (between RTAO and denoise)
        const vulkan::Image* aoInputForDenoise = nullptr;
        if (_temporalAccumulator)
        {
            auto projMat = _scene->mainCamera()->projectionMatrix();
            auto viewMat = _scene->mainCamera()->viewMatrix();
            auto invVP = glm::inverse(projMat * viewMat);

            auto aoImg = _rtAmbientOcclusion->aoImage(frameResourcesIndex);
            _temporalAccumulator->render(
                cmd, currentFrame, frameResources, gbuffer,
                aoImg.get(), invVP, viewMat, projMat
            );
            aoInputForDenoise = _temporalAccumulator->outputImage(frameResourcesIndex).get();
        }
        else
        {
            aoInputForDenoise = _rtAmbientOcclusion->aoImage(frameResourcesIndex).get();
        }

        // Denoise pass: filter the AO image
        {
            _denoiseFilter->render(cmd, currentFrame, frameResources, gbuffer, aoInputForDenoise);
        }

        // RT Reflections pass (after denoise, before composite)
        if (_rtReflections && _rtReflections->rtSupported())
        {
            auto tlas = frameResources.rayTracingScene ?
                frameResources.rayTracingScene->tlas() : VK_NULL_HANDLE;

            if (tlas != VK_NULL_HANDLE && _rtReflections->settings().enabled)
            {
                const auto& objectInstances = frameResources.rayTracingScene->objectInstances();

                _rtReflections->render(
                    cmd, currentFrame, frameResources, gbuffer,
                    invVP, cameraWorldPos, tlas, objectInstances,
                    _environment->irradianceMapImage().get(),
                    _environment->irradianceMapSampler(),
                    _reflectionLights
                );

                // Reflection temporal accumulation
                auto rawReflectionImage = _rtReflections->reflectionImage(frameResourcesIndex);
                _temporalReflectionAccumulator->render(
                    cmd, currentFrame, frameResources, gbuffer,
                    rawReflectionImage.get(), invVP, viewMat, projMat
                );

                reflectionInputForComposite = _temporalReflectionAccumulator->outputImage(frameResourcesIndex).get();
            }
        }
    }

    if (_debugVisualization == DeferredDebugVisualization::FullComposition)
    {
        renderCompositePass(cmd, currentFrame, inputImage, outputImage, frameResources, viewMatrix, projMatrix, reflectionInputForComposite);
    }
    else
    {
        auto* src = resolveDebugSource(inputImage, gbuffer);
        if (src)
        {
            uint32_t channelMode = 0;
            if (_debugVisualization == DeferredDebugVisualization::RTReflectionMask)
            {
                channelMode = 4;
            }
            renderDebugPass(cmd, src, outputImage, frameResources, channelMode);
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
    if (_temporalAccumulator) _temporalAccumulator->resize(newExtent);
    _denoiseFilter->resize(newExtent);
    if (_rtReflections) _rtReflections->resize(newExtent);
    if (_temporalReflectionAccumulator) _temporalReflectionAccumulator->resize(newExtent);
}

void DeferredLayer::cleanup()
{
    for (auto& gb : _gbuffers)
    {
        gb->cleanup();
    }
    _gbuffers.clear();

    if (_rtAmbientOcclusion) _rtAmbientOcclusion->cleanup();
    if (_temporalAccumulator) _temporalAccumulator->cleanup();
    if (_denoiseFilter) _denoiseFilter->cleanup();
    if (_temporalReflectionAccumulator) _temporalReflectionAccumulator->cleanup();
    if (_rtReflections) _rtReflections->cleanup();
    if (_rtMaterialDataBinding) _rtMaterialDataBinding->cleanup();
    if (_rtReflectionFallbackImage) _rtReflectionFallbackImage->cleanup();

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

void DeferredLayer::setAOQuality(RTAOQuality quality)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setQuality(quality);
}

RTAOQuality DeferredLayer::aoQuality() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->quality() : RTAOQuality::High;
}

void DeferredLayer::setAOSampleCount(int count)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAOSampleCount(count);
}

int DeferredLayer::aoSampleCount() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoSampleCount() : 6;
}

void DeferredLayer::setAOBounceCount(int count)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAOBounceCount(count);
}

int DeferredLayer::aoBounceCount() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoBounceCount() : 3;
}

void DeferredLayer::setAORadius(float radius)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAORadius(radius);
}

float DeferredLayer::aoRadius() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoRadius() : 0.3f;
}

void DeferredLayer::setAOBias(float bias)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAOBias(bias);
}

float DeferredLayer::aoBias() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoBias() : 0.01f;
}

void DeferredLayer::setAOFalloff(float falloff)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAOFalloff(falloff);
}

float DeferredLayer::aoFalloff() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoFalloff() : 1.0f;
}

void DeferredLayer::setAOBounceAttenuation(float attenuation)
{
    if (_rtAmbientOcclusion) _rtAmbientOcclusion->setAOBounceAttenuation(attenuation);
}

float DeferredLayer::aoBounceAttenuation() const
{
    return _rtAmbientOcclusion ? _rtAmbientOcclusion->aoBounceAttenuation() : 0.5f;
}

void DeferredLayer::setTemporalHistoryWeight(float weight)
{
    if (_temporalAccumulator) _temporalAccumulator->setHistoryWeight(weight);
}

float DeferredLayer::temporalHistoryWeight() const
{
    return _temporalAccumulator ? _temporalAccumulator->historyWeight() : 0.9f;
}

void DeferredLayer::setTemporalDepthThreshold(float threshold)
{
    if (_temporalAccumulator) _temporalAccumulator->setDepthThreshold(threshold);
}

float DeferredLayer::temporalDepthThreshold() const
{
    return _temporalAccumulator ? _temporalAccumulator->depthThreshold() : 0.01f;
}

void DeferredLayer::setTemporalNormalThreshold(float threshold)
{
    if (_temporalAccumulator) _temporalAccumulator->setNormalThreshold(threshold);
}

float DeferredLayer::temporalNormalThreshold() const
{
    return _temporalAccumulator ? _temporalAccumulator->normalThreshold() : 0.8f;
}

void DeferredLayer::setTemporalMode(TemporalAccumulator::AccumulationMode mode)
{
    if (_temporalAccumulator) _temporalAccumulator->setAccumulationMode(mode);
}

TemporalAccumulator::AccumulationMode DeferredLayer::temporalMode() const
{
    return _temporalAccumulator ? _temporalAccumulator->accumulationMode() : TemporalAccumulator::AccumulationMode::Interactive;
}

void DeferredLayer::setDenoiseKernelRadius(int radius)
{
    if (_denoiseFilter) _denoiseFilter->setKernelRadius(radius);
}

int DeferredLayer::denoiseKernelRadius() const
{
    return _denoiseFilter ? _denoiseFilter->kernelRadius() : 3;
}

void DeferredLayer::setDenoiseDepthThreshold(float threshold)
{
    if (_denoiseFilter) _denoiseFilter->setDepthThreshold(threshold);
}

float DeferredLayer::denoiseDepthThreshold() const
{
    return _denoiseFilter ? _denoiseFilter->depthThreshold() : 0.01f;
}

void DeferredLayer::setDenoiseNormalThreshold(float threshold)
{
    if (_denoiseFilter) _denoiseFilter->setNormalThreshold(threshold);
}

float DeferredLayer::denoiseNormalThreshold() const
{
    return _denoiseFilter ? _denoiseFilter->normalThreshold() : 0.8f;
}

void DeferredLayer::setDenoiseDepthSigma(float sigma)
{
    if (_denoiseFilter) _denoiseFilter->setDepthSigma(sigma);
}

float DeferredLayer::denoiseDepthSigma() const
{
    return _denoiseFilter ? _denoiseFilter->depthSigma() : 0.01f;
}

void DeferredLayer::setDenoiseNormalSigma(float sigma)
{
    if (_denoiseFilter) _denoiseFilter->setNormalSigma(sigma);
}

float DeferredLayer::denoiseNormalSigma() const
{
    return _denoiseFilter ? _denoiseFilter->normalSigma() : 0.3f;
}

void DeferredLayer::setRTReflectionsEnabled(bool enabled)
{
    if (_rtReflections) _rtReflections->setEnabled(enabled);
}

bool DeferredLayer::rtReflectionsEnabled() const
{
    return _rtReflections ? _rtReflections->settings().enabled : false;
}

void DeferredLayer::setRTReflectionSampleCount(uint32_t count)
{
    if (_rtReflections) _rtReflections->setSampleCount(count);
}

uint32_t DeferredLayer::rtReflectionSampleCount() const
{
    return _rtReflections ? _rtReflections->settings().sampleCount : 4;
}

void DeferredLayer::setRTReflectionMaxRoughness(float r)
{
    if (_rtReflections) _rtReflections->setMaxRoughness(r);
}

float DeferredLayer::rtReflectionMaxRoughness() const
{
    return _rtReflections ? _rtReflections->settings().maxRoughness : 0.35f;
}

void DeferredLayer::setRTReflectionRayBias(float b)
{
    if (_rtReflections) _rtReflections->setRayBias(b);
}

float DeferredLayer::rtReflectionRayBias() const
{
    return _rtReflections ? _rtReflections->settings().rayBias : 0.02f;
}

void DeferredLayer::setRTReflectionMaxDistance(float d)
{
    if (_rtReflections) _rtReflections->setMaxDistance(d);
}

float DeferredLayer::rtReflectionMaxDistance() const
{
    return _rtReflections ? _rtReflections->settings().maxDistance : 50.0f;
}

void DeferredLayer::setRTReflectionRoughnessSpread(float s)
{
    if (_rtReflections) _rtReflections->setRoughnessSpread(s);
}

float DeferredLayer::rtReflectionRoughnessSpread() const
{
    return _rtReflections ? _rtReflections->settings().roughnessSpread : 1.0f;
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
    _gbufferPipelineLayout = layoutFactory.build("DeferredLayer::GBufferPipelineLayout");

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

    _gbufferPipeline = plFactory.build(_gbufferPipelineLayout, "DeferredLayer::GBufferPipeline");

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
//    dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO
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
    _compositePipelineLayout = layoutFactory.build("DeferredLayer::CompositePipelineLayout");

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

    _compositePipeline = plFactory.build(_compositePipelineLayout, "DeferredLayer::CompositePipeline");

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _compositePipeline, nullptr);
        vkDestroyPipelineLayout(dev, _compositePipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _compositeGBufferDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _compositeGBufferRTDSLayout, nullptr);
    });
}

void DeferredLayer::createCompositePipelineRT()
{
    if (!_useRtShadows || !_rtDataBinding)
    {
        return;
    }

    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Albedo
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Normal
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Material
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_FresnelFlags
    dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_SheenColor
    dsLayoutFactory.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_InputImage
    dsLayoutFactory.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Depth
    dsLayoutFactory.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_AO
    dsLayoutFactory.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_RTReflection
    _compositeGBufferRTDSLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    // Create pipeline layout with RT data binding
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_compositeGBufferRTDSLayout);
    layoutFactory.addDescriptorSetLayout(_fragmentFrameDataBinding->createLayout(VK_SHADER_STAGE_FRAGMENT_BIT));
    layoutFactory.addDescriptorSetLayout(_environmentDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_lightDataBinding->createLayout());
    layoutFactory.addDescriptorSetLayout(_rtDataBinding->createLayout());

    layoutFactory.addPushConstantRange(
        0,
        sizeof(CompositePushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _compositePipelineRTLayout = layoutFactory.build("DeferredLayer::CompositeRTPipelineLayout");

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

    _compositePipelineRT = plFactory.build(_compositePipelineRTLayout, "DeferredLayer::CompositeRTPipeline");

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
    layoutFactory.addPushConstantRange(
        0,
        sizeof(DebugPushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _debugPipelineLayout = layoutFactory.build("DeferredLayer::DebugPipelineLayout");

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

    _debugPipeline = plFactory.build(_debugPipelineLayout, "DeferredLayer::DebugPipeline");

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
    const glm::mat4& projMatrix,
    const vulkan::Image* reflectionImage
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

    // Select pipeline variant
    bool useRT = _useRtShadows && tlas != VK_NULL_HANDLE;

    VkPipeline activePipeline = useRT ? _compositePipelineRT : _compositePipeline;
    VkPipelineLayout activeLayout = useRT ? _compositePipelineRTLayout : _compositePipelineLayout;
    VkDescriptorSetLayout activeGBufferLayout = useRT ? _compositeGBufferRTDSLayout : _compositeGBufferDSLayout;

    // Bind composite pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

    // Create G-buffer descriptor set
    auto gbufferDS = frameResources.newDescriptorSet(activeGBufferLayout);
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
    if (useRT)
    {
        auto denoisedAoImg = _denoiseFilter->outputImage(_engine->currentFrameResourcesIndex());
        gbufferDS->addImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            denoisedAoImg.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);

        const vulkan::Image* reflImg = reflectionImage ? reflectionImage : _rtReflectionFallbackImage.get();
        gbufferDS->addImage(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            reflImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _gbufferSampler);
    }
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
    vulkan::FrameResources& frameResources,
    uint32_t channelMode
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

    // Push constants for channel mode
    const DebugPushConstants pc { channelMode };
    vkCmdPushConstants(
        cmd, _debugPipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(DebugPushConstants),
        &pc
    );

    // 6. Draw fullscreen quad
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // 7. End rendering
    vulkan::cmdEndRendering(cmd);
}

}
