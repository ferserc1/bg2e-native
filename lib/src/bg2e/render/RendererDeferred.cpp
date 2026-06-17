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

#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e::render {

RendererDeferred::~RendererDeferred()
{
}

void RendererDeferred::build(
    bg2e::render::Engine* engine,
    VkExtent2D initialExtent,
    VkFormat colorImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits, // Ignore swap chain sample count
    bool isOffscreen
) {
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    _engine = engine;
    _viewportExtent = initialExtent;
    _colorImageFormat = colorImageFormat;
    _depthImageFormat = depthImageFormat;
    _sampleCount = sampleCount;
    _isOffscreen = isOffscreen;

    _scene = std::make_unique<bg2e::scene::Scene>();

    _environment = std::unique_ptr<EnvironmentResources>(
        new EnvironmentResources(
            _engine,
            { colorImageFormat },
            depthImageFormat,
            sampleCount
        )
    );

    // Create shared data bindings for deferred layers
    _lightDataBinding = std::make_unique<scene::vk::DeferredLightDataBinding>(_engine);
    if (_engine->rayTracingSupported()) {
        _rtDataBinding = std::make_unique<vulkan::rt::RayTracingSceneDataBinding>(_engine);
        _reflectionLightDataBinding = std::make_unique<vulkan::rt::ReflectionLightDataBinding>(_engine);
    }

    // Create layers
    _skyboxLayer = std::make_unique<deferred::SkyboxLayer>(_engine);
    _skyboxLayer->build(initialExtent, colorImageFormat);
    _skyboxLayer->setScene(_scene.get());
    _skyboxLayer->setEnvironment(_environment.get());

    _opaqueLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Opaque);
    _opaqueLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _opaqueLayer->setRtDataBinding(_rtDataBinding.get());
    if (_reflectionLightDataBinding) _opaqueLayer->setReflectionLightDataBinding(_reflectionLightDataBinding.get());
    _opaqueLayer->build(initialExtent, colorImageFormat);
    _opaqueLayer->setScene(_scene.get());
    _opaqueLayer->setEnvironment(_environment.get());
    _opaqueLayer->setRenderQueue(&_renderQueue);


    _transparentLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Transparent);
    _transparentLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _transparentLayer->setRtDataBinding(_rtDataBinding.get());
    if (_reflectionLightDataBinding) _transparentLayer->setReflectionLightDataBinding(_reflectionLightDataBinding.get());
    _transparentLayer->build(initialExtent, colorImageFormat);
    _transparentLayer->setScene(_scene.get());
    _transparentLayer->setEnvironment(_environment.get());
    _transparentLayer->setRenderQueue(&_renderQueue);
    _transparentLayer->setIsTransparent(true);


    // Create intermediate images
    _skyboxImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "deferred render skybox image",
            colorImageFormat,
            initialExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    _opaqueImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render opaque image",
            colorImageFormat,
            initialExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    // Intermediate image for the transparent layer output. SMAA reads from this
    // image and writes the anti-aliased result into the final colorImage, so the
    // transparent layer must not render directly into colorImage (which may be a
    // swapchain image and is also the SMAA copy destination).
    _transparentImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render transparent image",
            colorImageFormat,
            initialExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    // GizmoAndSelectionRenderer (non-offscreen only)
    if (!isOffscreen) {
        _gizmoAndSelectionRenderer = std::make_unique<manipulation::GizmoAndSelectionRenderer>();
        _gizmoAndSelectionRenderer->init(engine, VK_SAMPLE_COUNT_1_BIT);
    }

    // SMAA post-processing
    _smaaProcessor = std::make_unique<deferred::SMAAProcessor>(_engine);
    _smaaProcessor->build(initialExtent, colorImageFormat);
}

void RendererDeferred::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
) {
    _environment->initFrameResources(frameAllocator);
    _lightDataBinding->initFrameResources(frameAllocator);
    if (_rtDataBinding) {
        _rtDataBinding->initFrameResources(frameAllocator);
    }
    if (_reflectionLightDataBinding) {
        _reflectionLightDataBinding->initFrameResources(frameAllocator);
    }
    _skyboxLayer->initFrameResources(frameAllocator);
    _opaqueLayer->initFrameResources(frameAllocator);
    _transparentLayer->initFrameResources(frameAllocator);
}

void RendererDeferred::initScene(
    std::shared_ptr<bg2e::scene::Node> sceneRoot
) {
    _scene->setSceneRoot(sceneRoot);

    auto skyDomeTexture = std::make_shared<bg2e::base::Texture>();
    auto skyDomeGenerator = new bg2e::scene::SkyDomeTextureGenerator(2048, 1024, 4);
    skyDomeTexture->setProceduralGenerator(skyDomeGenerator);
    skyDomeTexture->setUseMipmaps(false);
    auto envTexture = std::make_shared<bg2e::render::Texture>(_engine);
    envTexture->load(skyDomeTexture);
    _environment->build(
        envTexture,
        { 2048, 2048 },
        { 32, 32 },
        { 1024, 1024 }
    );

    _scene->updateLights();

    _resizeVisitor.resizeViewport(_scene->rootNode(), _viewportExtent);

    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}

void RendererDeferred::resize(
    VkExtent2D newExtent
) {
    _viewportExtent = newExtent;
    _scene->willResize();

    // Resize layers
    _skyboxLayer->resize(newExtent);
    _opaqueLayer->resize(newExtent);
    _transparentLayer->resize(newExtent);

    // Recreate intermediate images
    _skyboxImage->cleanup();
    _skyboxImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "deferred render skybox image",
            _colorImageFormat,
            newExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            false,
            20,
            VK_SAMPLE_COUNT_1_BIT
        )
    );
    _opaqueImage->cleanup();
    _opaqueImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render opaque image",
            _colorImageFormat,
            newExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            false,
            20,
            VK_SAMPLE_COUNT_1_BIT
        )
    );
    _transparentImage->cleanup();
    _transparentImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render transparent image",
            _colorImageFormat,
            newExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            false,
            20,
            VK_SAMPLE_COUNT_1_BIT
        )
    );

    if (_smaaProcessor) {
        _smaaProcessor->resize(newExtent);
    }

    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);

    _scene->didResize();
}

void RendererDeferred::update(
    float delta
) {
    updateScene(delta, BG2E_MAX_DEFERRED_LIGHTS);
}

void RendererDeferred::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* colorImage,
    [[maybe_unused]] const bg2e::render::vulkan::Image* depthImage,
    [[maybe_unused]] const bg2e::render::vulkan::Image* msaaDepthImage,
    bg2e::render::vulkan::FrameResources& frameResources,
    VkImageLayout & outColorImageLayout,
    VkImageLayout & outDepthImageLayout,
    VkImageLayout & outMsaaDepthImageLayout
) {
    // === Scene preparation (from Renderer base) ===
    prepareSceneRender(cmd, currentFrame, frameResources);

    // Configure light uniforms on deferred layers
    _opaqueLayer->setLights(_lights);
    _transparentLayer->setLights(_lights);

    // Build the reduced light set that affects ray traced reflections, so the
    // reflection shader only iterates over the lights flagged for it.
    if (_reflectionLightDataBinding)
    {
        _reflectionLights.clear();
        for (const auto& light : _lights)
        {
            if (light.affectsReflections != 0)
            {
                _reflectionLights.push_back(light);
            }
        }
        _opaqueLayer->setReflectionLights(_reflectionLights);
        _transparentLayer->setReflectionLights(_reflectionLights);
    }

    // Configure color correction on all layers
    _skyboxLayer->setColorCorrection(_brightness, _contrast, _exposure);
    _opaqueLayer->setColorCorrection(_brightness, _contrast, _exposure);
    _transparentLayer->setColorCorrection(_brightness, _contrast, _exposure);

    // Layer 1: Skybox
    _skyboxLayer->render(
        cmd,
        currentFrame,
        nullptr,
        _skyboxImage.get(),
        frameResources
    );

    vulkan::Image::cmdTransitionImage(cmd, _skyboxImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Layer 2: Opaque
    _opaqueLayer->render(
        cmd,
        currentFrame,
        _skyboxImage.get(),
        _opaqueImage.get(),
        frameResources
    );

    vulkan::Image::cmdTransitionImage(
        cmd,
        _opaqueImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Layer 3: Transparent (writes to the intermediate transparent image, not
    // to the final colorImage, so SMAA can sample it as input)
    _transparentLayer->setOpaqueDepthBuffer(_opaqueLayer->depthBuffer());
    _transparentLayer->render(cmd, currentFrame, _opaqueImage.get(), _transparentImage.get(),
                              frameResources);

    if (!_isOffscreen && _gizmoAndSelectionRenderer) {
        auto depthAttachment = vulkan::Info::depthAttachmentInfo(depthImage->imageView(), 1.0f);
        auto colorAttachment = vulkan::Info::attachmentInfo(_transparentImage->imageView(), nullptr);
        auto renderInfo = vulkan::Info::renderingInfo(_transparentImage->extent2D(), &colorAttachment, nullptr);
        vulkan::cmdBeginRendering(cmd, &renderInfo);

        vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _transparentImage->extent2D());

        auto mainCamera = _scene->mainCamera();
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        auto projMatrix = mainCamera->projectionMatrix();
        _gizmoAndSelectionRenderer->draw(_scene->rootNode(), viewMatrix, projMatrix, cmd);

        vulkan::cmdEndRendering(cmd);
    }

    // === SMAA Anti-Aliasing ===
    // Input: the intermediate transparent image. Output: copied into colorImage.
    if (_smaaProcessor) {
        vulkan::Image::cmdTransitionImage(
            cmd,
            _transparentImage->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        const vulkan::Image* smaaOutput = _smaaProcessor->process(
            cmd, currentFrame, _transparentImage.get()
        );

        vulkan::Image::cmdCopy(
            cmd,
            smaaOutput->handle(),
            smaaOutput->extent2D(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            colorImage->handle(),
            colorImage->extent2D(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }

    // === End scene render (from Renderer base) ===
    endSceneRender();

    vulkan::Image::cmdTransitionImage(
        cmd,
        colorImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    outColorImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    outDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    outMsaaDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
}

void RendererDeferred::cleanup() {
    if (_smaaProcessor) {
        _smaaProcessor->cleanup();
        _smaaProcessor.reset();
    }

    _gizmoAndSelectionRenderer.reset();

    _transparentLayer->cleanup();
    _opaqueLayer->cleanup();
    _skyboxLayer->cleanup();

    if (_transparentImage)
    {
        _transparentImage->cleanup();
    }
    _transparentImage.reset();

    if (_opaqueImage)
    {
        _opaqueImage->cleanup();
    }
    _opaqueImage.reset();

    if (_skyboxImage)
    {
        _skyboxImage->cleanup();
    }
    _skyboxImage.reset();

    if (_rtDataBinding)
    {
        _rtDataBinding->cleanup();
    }
    if (_reflectionLightDataBinding)
    {
        _reflectionLightDataBinding->cleanup();
    }
    _lightDataBinding->cleanup();
    _rtDataBinding.reset();
    _reflectionLightDataBinding.reset();
    _lightDataBinding.reset();

    _renderQueue.cleanup();
}

deferred::DeferredDebugVisualization RendererDeferred::debugVisualization() const
{
    return _debugVisualization;
}

void RendererDeferred::setDebugVisualization(deferred::DeferredDebugVisualization debugVisualization)
{
    _debugVisualization = debugVisualization;
    _opaqueLayer->setDebugVisualization(debugVisualization);
    _transparentLayer->setDebugVisualization(debugVisualization);
}

void RendererDeferred::setAOQuality(deferred::RTAOQuality quality)
{
    _opaqueLayer->setAOQuality(quality);
    _transparentLayer->setAOQuality(quality);
}

deferred::RTAOQuality RendererDeferred::aoQuality() const
{
    return _opaqueLayer->aoQuality();
}

void RendererDeferred::setAOSampleCount(int count)
{
    _opaqueLayer->setAOSampleCount(count);
    _transparentLayer->setAOSampleCount(count);
}

int RendererDeferred::aoSampleCount() const
{
    return _opaqueLayer->aoSampleCount();
}

void RendererDeferred::setAOBounceCount(int count)
{
    _opaqueLayer->setAOBounceCount(count);
    _transparentLayer->setAOBounceCount(count);
}

int RendererDeferred::aoBounceCount() const
{
    return _opaqueLayer->aoBounceCount();
}

void RendererDeferred::setAORadius(float radius)
{
    _opaqueLayer->setAORadius(radius);
    _transparentLayer->setAORadius(radius);
}

float RendererDeferred::aoRadius() const
{
    return _opaqueLayer->aoRadius();
}

void RendererDeferred::setAOBias(float bias)
{
    _opaqueLayer->setAOBias(bias);
    _transparentLayer->setAOBias(bias);
}

float RendererDeferred::aoBias() const
{
    return _opaqueLayer->aoBias();
}

void RendererDeferred::setAOFalloff(float falloff)
{
    _opaqueLayer->setAOFalloff(falloff);
    _transparentLayer->setAOFalloff(falloff);
}

float RendererDeferred::aoFalloff() const
{
    return _opaqueLayer->aoFalloff();
}

void RendererDeferred::setAOBounceAttenuation(float attenuation)
{
    _opaqueLayer->setAOBounceAttenuation(attenuation);
    _transparentLayer->setAOBounceAttenuation(attenuation);
}

float RendererDeferred::aoBounceAttenuation() const
{
    return _opaqueLayer->aoBounceAttenuation();
}

void RendererDeferred::setTemporalHistoryWeight(float weight)
{
    _opaqueLayer->setTemporalHistoryWeight(weight);
    _transparentLayer->setTemporalHistoryWeight(weight);
}

float RendererDeferred::temporalHistoryWeight() const
{
    return _opaqueLayer->temporalHistoryWeight();
}

void RendererDeferred::setTemporalDepthThreshold(float threshold)
{
    _opaqueLayer->setTemporalDepthThreshold(threshold);
    _transparentLayer->setTemporalDepthThreshold(threshold);
}

float RendererDeferred::temporalDepthThreshold() const
{
    return _opaqueLayer->temporalDepthThreshold();
}

void RendererDeferred::setTemporalNormalThreshold(float threshold)
{
    _opaqueLayer->setTemporalNormalThreshold(threshold);
    _transparentLayer->setTemporalNormalThreshold(threshold);
}

float RendererDeferred::temporalNormalThreshold() const
{
    return _opaqueLayer->temporalNormalThreshold();
}

void RendererDeferred::setTemporalMode(deferred::TemporalAccumulator::AccumulationMode mode)
{
    _opaqueLayer->setTemporalMode(mode);
    _transparentLayer->setTemporalMode(mode);
}

deferred::TemporalAccumulator::AccumulationMode RendererDeferred::temporalMode() const
{
    return _opaqueLayer->temporalMode();
}

void RendererDeferred::setDenoiseKernelRadius(int radius)
{
    _opaqueLayer->setDenoiseKernelRadius(radius);
    _transparentLayer->setDenoiseKernelRadius(radius);
}

int RendererDeferred::denoiseKernelRadius() const
{
    return _opaqueLayer->denoiseKernelRadius();
}

void RendererDeferred::setDenoiseDepthThreshold(float threshold)
{
    _opaqueLayer->setDenoiseDepthThreshold(threshold);
    _transparentLayer->setDenoiseDepthThreshold(threshold);
}

float RendererDeferred::denoiseDepthThreshold() const
{
    return _opaqueLayer->denoiseDepthThreshold();
}

void RendererDeferred::setDenoiseNormalThreshold(float threshold)
{
    _opaqueLayer->setDenoiseNormalThreshold(threshold);
    _transparentLayer->setDenoiseNormalThreshold(threshold);
}

float RendererDeferred::denoiseNormalThreshold() const
{
    return _opaqueLayer->denoiseNormalThreshold();
}

void RendererDeferred::setDenoiseDepthSigma(float sigma)
{
    _opaqueLayer->setDenoiseDepthSigma(sigma);
    _transparentLayer->setDenoiseDepthSigma(sigma);
}

float RendererDeferred::denoiseDepthSigma() const
{
    return _opaqueLayer->denoiseDepthSigma();
}

void RendererDeferred::setDenoiseNormalSigma(float sigma)
{
    _opaqueLayer->setDenoiseNormalSigma(sigma);
    _transparentLayer->setDenoiseNormalSigma(sigma);
}

float RendererDeferred::denoiseNormalSigma() const
{
    return _opaqueLayer->denoiseNormalSigma();
}

// RT Reflections

void RendererDeferred::setRTReflectionsEnabled(bool enabled)
{
    _opaqueLayer->setRTReflectionsEnabled(enabled);
    _transparentLayer->setRTReflectionsEnabled(enabled);
}

bool RendererDeferred::rtReflectionsEnabled() const
{
    return _opaqueLayer->rtReflectionsEnabled();
}

void RendererDeferred::setRTReflectionSampleCount(uint32_t count)
{
    _opaqueLayer->setRTReflectionSampleCount(count);
    _transparentLayer->setRTReflectionSampleCount(count);
}

uint32_t RendererDeferred::rtReflectionSampleCount() const
{
    return _opaqueLayer->rtReflectionSampleCount();
}

void RendererDeferred::setRTReflectionMaxRoughness(float roughness)
{
    _opaqueLayer->setRTReflectionMaxRoughness(roughness);
    _transparentLayer->setRTReflectionMaxRoughness(roughness);
}

float RendererDeferred::rtReflectionMaxRoughness() const
{
    return _opaqueLayer->rtReflectionMaxRoughness();
}

void RendererDeferred::setRTReflectionRayBias(float bias)
{
    _opaqueLayer->setRTReflectionRayBias(bias);
    _transparentLayer->setRTReflectionRayBias(bias);
}

float RendererDeferred::rtReflectionRayBias() const
{
    return _opaqueLayer->rtReflectionRayBias();
}

void RendererDeferred::setRTReflectionMaxDistance(float distance)
{
    _opaqueLayer->setRTReflectionMaxDistance(distance);
    _transparentLayer->setRTReflectionMaxDistance(distance);
}

float RendererDeferred::rtReflectionMaxDistance() const
{
    return _opaqueLayer->rtReflectionMaxDistance();
}

void RendererDeferred::setRTReflectionRoughnessSpread(float spread)
{
    _opaqueLayer->setRTReflectionRoughnessSpread(spread);
    _transparentLayer->setRTReflectionRoughnessSpread(spread);
}

float RendererDeferred::rtReflectionRoughnessSpread() const
{
    return _opaqueLayer->rtReflectionRoughnessSpread();
}

void RendererDeferred::updateLights(
    const std::vector<std::shared_ptr<bg2e::scene::LightComponent>>& lightComponents,
    [[maybe_unused]] uint32_t maxLights
) {
    _lights.clear();
    _lights.resize(lightComponents.size());

    // Deferred render: maxLight is ignored

    for (auto & comp : lightComponents)
    {
        _lights.push_back({
            .position = comp->position(),
            .intensity = comp->light().intensity(),
            .color = comp->light().color(),
            .direction = comp->direction(),
            .type = comp->light().type(),
            .spotAngle = comp->light().spotAngle(),
            .spotCutoff = comp->light().spotCutoff(),
            .castShadows = comp->light().castShadows() ? 1 : 0,
            .sourceSize = comp->light().sourceSize(),
            .shadowSamples = static_cast<int32_t>(comp->light().shadowSamples()),
            .affectsReflections = comp->light().affectsReflections() ? 1 : 0
        });
    }
}

}
