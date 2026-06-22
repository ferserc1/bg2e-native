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
#include <bg2e/render/deferred/SMAAPostProcessor.hpp>
#if !defined(__APPLE__)
#include <bg2e/render/deferred/FSRPostProcessor.hpp>
#endif

#include <bg2e/math/projections.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace bg2e::render {

RendererDeferred::~RendererDeferred()
{
}

VkExtent2D RendererDeferred::computeRenderExtent(VkExtent2D viewportExtent, float scalePercent) const
{
    uint32_t w = static_cast<uint32_t>(std::round(viewportExtent.width * scalePercent / 100.0f));
    uint32_t h = static_cast<uint32_t>(std::round(viewportExtent.height * scalePercent / 100.0f));
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    return { w, h };
}

void RendererDeferred::setRenderScalePercent(float percent)
{
    _engine->device().waitIdle();
    if (percent < 25.0f) percent = 25.0f;
    if (percent > 150.0f) percent = 150.0f;
    if (percent == _renderScalePercent) return;

    _renderScalePercent = percent;

    if (!_engine) return;

    auto newRenderExtent = computeRenderExtent(_viewportExtent, _renderScalePercent);
    if (newRenderExtent.width == _renderExtent.width && newRenderExtent.height == _renderExtent.height)
    {
        return;
    }

    _renderExtent = newRenderExtent;
    _scene->willResize();

    _skyboxLayer->resize(_renderExtent);
    _opaqueLayer->resize(_renderExtent);
    _transparentLayer->resize(_renderExtent);

    _skyboxImage->cleanup();
    _skyboxImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "deferred render skybox image",
            _colorImageFormat,
            _renderExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    _opaqueImage->cleanup();
    _opaqueImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render opaque image",
            _colorImageFormat,
            _renderExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    _transparentImage->cleanup();
    _transparentImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred render transparent image",
            _colorImageFormat,
            _renderExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );

    createGizmoDepthImage();

    if (_motionVectorGenerator) _motionVectorGenerator->resize(_renderExtent);
    if (_finalPostProcessor)    _finalPostProcessor->resize(_renderExtent, _viewportExtent);

    _scene->didResize();
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
    _renderScalePercent = 50.0f;
    _renderExtent = computeRenderExtent(initialExtent, _renderScalePercent);
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
    _skyboxLayer->build(_renderExtent, colorImageFormat);
    _skyboxLayer->setScene(_scene.get());
    _skyboxLayer->setEnvironment(_environment.get());

    _opaqueLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Opaque);
    _opaqueLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _opaqueLayer->setRtDataBinding(_rtDataBinding.get());
    if (_reflectionLightDataBinding) _opaqueLayer->setReflectionLightDataBinding(_reflectionLightDataBinding.get());
    _opaqueLayer->build(_renderExtent, colorImageFormat);
    _opaqueLayer->setScene(_scene.get());
    _opaqueLayer->setEnvironment(_environment.get());
    _opaqueLayer->setRenderQueue(&_renderQueue);


    _transparentLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Transparent);
    _transparentLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _transparentLayer->setRtDataBinding(_rtDataBinding.get());
    if (_reflectionLightDataBinding) _transparentLayer->setReflectionLightDataBinding(_reflectionLightDataBinding.get());
    _transparentLayer->build(_renderExtent, colorImageFormat);
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
            _renderExtent,
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
            _renderExtent,
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
            _renderExtent,
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
        createGizmoDepthImage();
    }

    // Motion vector generator (needed by FSR for temporal upscaling)
    _motionVectorGenerator = std::make_unique<deferred::MotionVectorGenerator>(_engine);
    _motionVectorGenerator->build(_renderExtent);

    // Final post-processor: FSR on Windows/Linux, SMAA everywhere else
#if !defined(__APPLE__)
    auto* fsrProc = new deferred::FSRPostProcessor();
    _finalPostProcessor = std::unique_ptr<deferred::FinalPostProcessor>(fsrProc);
#else
    auto* smaaProc = new deferred::SMAAPostProcessor();
    _finalPostProcessor = std::unique_ptr<deferred::FinalPostProcessor>(smaaProc);
#endif
    _finalPostProcessor->build(_engine, _renderExtent, _viewportExtent, colorImageFormat);
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

    auto newRenderExtent = computeRenderExtent(_viewportExtent, _renderScalePercent);
    if (newRenderExtent.width == _renderExtent.width && newRenderExtent.height == _renderExtent.height)
    {
        _resizeVisitor.resizeViewport(_scene->rootNode(), _viewportExtent);
        return;
    }

    _renderExtent = newRenderExtent;
    _scene->willResize();

    // Resize layers at render resolution
    _skyboxLayer->resize(_renderExtent);
    _opaqueLayer->resize(_renderExtent);
    _transparentLayer->resize(_renderExtent);

    // Recreate intermediate images at render resolution
    _skyboxImage->cleanup();
    _skyboxImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "deferred render skybox image",
            _colorImageFormat,
            _renderExtent,
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
            _renderExtent,
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
            _renderExtent,
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

    createGizmoDepthImage();

    if (_motionVectorGenerator) _motionVectorGenerator->resize(_renderExtent);
    if (_finalPostProcessor)    _finalPostProcessor->resize(_renderExtent, _viewportExtent);

    _resizeVisitor.resizeViewport(_scene->rootNode(), _viewportExtent);

    _scene->didResize();
}

void RendererDeferred::update(
    float delta
) {
    _deltaTimeMs = delta * 1000.0f;
    updateScene(delta, BG2E_MAX_DEFERRED_LIGHTS);
}

void RendererDeferred::createGizmoDepthImage()
{
    // Editor-only: gizmos are not drawn offscreen.
    if (_isOffscreen)
    {
        return;
    }

    if (_gizmoDepthImage)
    {
        _gizmoDepthImage->cleanup();
    }

    _gizmoDepthImage = std::shared_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            "Deferred gizmo depth image",
            _depthImageFormat,
            _renderExtent,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );
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
    // --- Gather camera matrices before prepareSceneRender modifies the skybox ---
    auto mainCamera = _scene->mainCamera();
    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
    auto origProj   = mainCamera->projectionMatrix();

    // Let the post-processor compute jitter and return the modified projection matrix.
    // SMAAPostProcessor returns origProj unchanged; FSRPostProcessor applies Halton jitter.
    auto jitteredProj = _finalPostProcessor->prepare(origProj, _frameCounter, _renderExtent);

    // === Scene preparation (from Renderer base) ===
    // prepareSceneRender calls _environment->updateSkybox(view, origProj).
    // We override it immediately after with the jittered version.
    prepareSceneRender(cmd, currentFrame, frameResources);
    if (drawSkybox())
    {
        _environment->updateSkybox(viewMatrix, jitteredProj);
    }

    // Apply jitter override to deferred layers (GBuffer vertex pass)
    _opaqueLayer->setProjectionOverride(&jitteredProj);
    _transparentLayer->setProjectionOverride(&jitteredProj);

    // Configure light uniforms on deferred layers
    _opaqueLayer->setLights(_lights);
    _transparentLayer->setLights(_lights);

    // Build the reduced light set that affects ray traced reflections
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
    _skyboxLayer->render(cmd, currentFrame, nullptr, _skyboxImage.get(), frameResources);

    vulkan::Image::cmdTransitionImage(cmd, _skyboxImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Layer 2: Opaque
    _opaqueLayer->render(cmd, currentFrame, _skyboxImage.get(), _opaqueImage.get(), frameResources);

    vulkan::Image::cmdTransitionImage(cmd, _opaqueImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Layer 3: Transparent
    _transparentLayer->setOpaqueDepthBuffer(_opaqueLayer->depthBuffer());
    _transparentLayer->render(cmd, currentFrame, _opaqueImage.get(), _transparentImage.get(),
                              frameResources);

    // Clear projection overrides after all deferred layers have rendered
    _opaqueLayer->setProjectionOverride(nullptr);
    _transparentLayer->setProjectionOverride(nullptr);

    // Gizmos (editor-only, non-offscreen)
    if (!_isOffscreen && _gizmoAndSelectionRenderer)
    {
        // Attach a dedicated depth buffer (cleared on load) so the transform
        // gizmo self-occludes and stays on top, without disturbing the scene
        // depth that FSR still needs for motion vectors below.
        vulkan::Image::cmdTransitionImage(
            cmd,
            _gizmoDepthImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
        );

        auto colorAttachment = vulkan::Info::attachmentInfo(_transparentImage->imageView(), nullptr);
        auto depthAttachment = vulkan::Info::depthAttachmentInfo(_gizmoDepthImage->imageView());
        auto renderInfo = vulkan::Info::renderingInfo(_transparentImage->extent2D(), &colorAttachment, &depthAttachment);
        vulkan::cmdBeginRendering(cmd, &renderInfo);
        vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _transparentImage->extent2D());

        // Use original (non-jittered) matrices for gizmos so they stay sharp
        _gizmoAndSelectionRenderer->draw(_scene->rootNode(), viewMatrix, origProj, cmd, _transparentImage->extent2D());

        vulkan::cmdEndRendering(cmd);
    }

    // === Motion vectors (needed by FSR; harmless with SMAA) ===
    const vulkan::Image* motionVectors = _motionVectorGenerator->generate(
        cmd, currentFrame,
        _transparentLayer->depthBuffer().get(),
        glm::inverse(origProj * viewMatrix),
        _prevProjMatrix * _prevViewMatrix
    );

    // === Final post-processing (SMAA or FSR) ===
    vulkan::Image::cmdTransitionImage(
        cmd, _transparentImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    float cameraNear = 0.1f, cameraFar = 1000.0f, cameraFovV = glm::radians(60.0f);
    if (auto* proj = mainCamera->projection())
    {
        cameraNear = proj->near();
        cameraFar  = proj->far();
        if (auto* persp = dynamic_cast<bg2e::math::PerspectiveProjection*>(proj))
            cameraFovV = glm::radians(persp->fov());
        else if (auto* optical = dynamic_cast<bg2e::math::OpticalProjection*>(proj))
            cameraFovV = 2.0f * std::atan(optical->frameSize() / (optical->focalLength() * 2.0f));
    }

    _finalPostProcessor->process(
        cmd, currentFrame,
        _transparentImage.get(),
        _transparentLayer->depthBuffer().get(),
        motionVectors,
        colorImage,
        _deltaTimeMs,
        cameraNear,
        cameraFar,
        cameraFovV
    );

    // process() guarantees colorImage is in COLOR_ATTACHMENT_OPTIMAL on exit

    // === End scene render (from Renderer base) ===
    endSceneRender();

    // Store camera state for next frame's motion vector generation
    _prevViewMatrix = viewMatrix;
    _prevProjMatrix = origProj;
    _frameCounter++;

    outColorImageLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    outDepthImageLayout     = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    outMsaaDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
}

void RendererDeferred::cleanup() {
    if (_finalPostProcessor) {
        _finalPostProcessor->cleanup();
        _finalPostProcessor.reset();
    }
    if (_motionVectorGenerator) {
        _motionVectorGenerator->cleanup();
        _motionVectorGenerator.reset();
    }

    _gizmoAndSelectionRenderer.reset();

    if (_gizmoDepthImage)
    {
        _gizmoDepthImage->cleanup();
    }
    _gizmoDepthImage.reset();

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

void RendererDeferred::setIndirectLightingMode(deferred::IndirectLightingMode mode)
{
    _opaqueLayer->setIndirectLightingMode(mode);
    _transparentLayer->setIndirectLightingMode(mode);
}

deferred::IndirectLightingMode RendererDeferred::indirectLightingMode() const
{
    return _opaqueLayer->indirectLightingMode();
}

void RendererDeferred::setRTGIEnabled(bool enabled)
{
    _opaqueLayer->setRTGIEnabled(enabled);
    _transparentLayer->setRTGIEnabled(enabled);
}

bool RendererDeferred::rtGIEnabled() const
{
    return _opaqueLayer->rtGIEnabled();
}

void RendererDeferred::setRTGISampleCount(uint32_t count)
{
    _opaqueLayer->setRTGISampleCount(count);
    _transparentLayer->setRTGISampleCount(count);
}

uint32_t RendererDeferred::rtGISampleCount() const
{
    return _opaqueLayer->rtGISampleCount();
}

void RendererDeferred::setRTGIBounceCount(uint32_t count)
{
    _opaqueLayer->setRTGIBounceCount(count);
    _transparentLayer->setRTGIBounceCount(count);
}

uint32_t RendererDeferred::rtGIBounceCount() const
{
    return _opaqueLayer->rtGIBounceCount();
}

void RendererDeferred::setRTGIRayBias(float bias)
{
    _opaqueLayer->setRTGIRayBias(bias);
    _transparentLayer->setRTGIRayBias(bias);
}

float RendererDeferred::rtGIRayBias() const
{
    return _opaqueLayer->rtGIRayBias();
}

void RendererDeferred::setRTGIMaxDistance(float distance)
{
    _opaqueLayer->setRTGIMaxDistance(distance);
    _transparentLayer->setRTGIMaxDistance(distance);
}

float RendererDeferred::rtGIMaxDistance() const
{
    return _opaqueLayer->rtGIMaxDistance();
}

void RendererDeferred::setRTGIQuality(deferred::RTGIQuality quality)
{
    _opaqueLayer->setRTGIQuality(quality);
    _transparentLayer->setRTGIQuality(quality);
}

deferred::RTGIQuality RendererDeferred::rtGIQuality() const
{
    return _opaqueLayer->rtGIQuality();
}

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

// --- Scale UI API ---

std::string RendererDeferred::scaleProcessorName() const
{
    return _finalPostProcessor ? _finalPostProcessor->processorName() : "Render Scale";
}

std::vector<std::string> RendererDeferred::scaleOptions() const
{
    if (_finalPostProcessor) return _finalPostProcessor->scaleOptions();
    return {};
}

void RendererDeferred::setScaleOption(uint32_t index)
{
    if (!_finalPostProcessor) return;
    _finalPostProcessor->setScaleOption(index);
    setRenderScalePercent(_finalPostProcessor->renderScalePercent());
}

uint32_t RendererDeferred::scaleOption() const
{
    return _finalPostProcessor ? _finalPostProcessor->scaleOption() : 0;
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
