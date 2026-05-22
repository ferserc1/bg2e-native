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
    }

    // Create layers
    _skyboxLayer = std::make_unique<deferred::SkyboxLayer>(_engine);
    _skyboxLayer->build(initialExtent, colorImageFormat);
    _skyboxLayer->setScene(_scene.get());
    _skyboxLayer->setEnvironment(_environment.get());

    _opaqueLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Opaque);
    _opaqueLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _opaqueLayer->setRtDataBinding(_rtDataBinding.get());
    _opaqueLayer->build(initialExtent, colorImageFormat);
    _opaqueLayer->setScene(_scene.get());
    _opaqueLayer->setEnvironment(_environment.get());
    _opaqueLayer->setRenderQueue(&_renderQueue);


    _transparentLayer = std::make_unique<deferred::DeferredLayer>(_engine, deferred::LayerType::Transparent);
    _transparentLayer->setLightDataBinding(_lightDataBinding.get());
    if (_rtDataBinding) _transparentLayer->setRtDataBinding(_rtDataBinding.get());
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

    // Selection highlight (non-offscreen only)
    if (!isOffscreen) {
        _selectionHighlight = std::make_unique<manipulation::SelectionHighlight>();
        _selectionHighlight->init(engine, VK_SAMPLE_COUNT_1_BIT);
    }
}

void RendererDeferred::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
) {
    _environment->initFrameResources(frameAllocator);
    _lightDataBinding->initFrameResources(frameAllocator);
    if (_rtDataBinding) {
        _rtDataBinding->initFrameResources(frameAllocator);
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

    // Layer 3: Transparent (writes to swapchain output)
    _transparentLayer->setOpaqueDepthBuffer(_opaqueLayer->depthBuffer());
    _transparentLayer->render(cmd, currentFrame, _opaqueImage.get(), colorImage,
                              frameResources);

    // Selection highlight (non-offscreen only)
    if (!_isOffscreen && _selectionHighlight) {
        // TODO: Prepare selectionHighlight to work with non-msaa images
        VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
        auto depthAttachment = vulkan::Info::depthAttachmentInfo(depthImage->imageView(), 1.0f);
        auto colorAttachment = vulkan::Info::attachmentInfo(colorImage->imageView(), nullptr);
        auto renderInfo = vulkan::Info::renderingInfo(colorImage->extent2D(), &colorAttachment, nullptr);
        vulkan::cmdBeginRendering(cmd, &renderInfo);

        vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());

        auto mainCamera = _scene->mainCamera();
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        auto projMatrix = mainCamera->projectionMatrix();
        _selectionHighlight->draw(_scene->rootNode(), viewMatrix, projMatrix, cmd);

        vulkan::cmdEndRendering(cmd);
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
    _selectionHighlight.reset();

    _transparentLayer->cleanup();
    _opaqueLayer->cleanup();
    _skyboxLayer->cleanup();

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

    _rtDataBinding->cleanup();
    _lightDataBinding->cleanup();
    _rtDataBinding.reset();
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
            .castShadows = comp->light().castShadows() ? 1 : 0
        });
    }
}

}
