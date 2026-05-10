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

namespace bg2e::render {

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

    _skyboxLayer = std::make_unique<deferred::SkyboxLayer>(_engine);
    _skyboxLayer->build(initialExtent, colorImageFormat);
    _skyboxLayer->setScene(_scene.get());
    _skyboxLayer->setEnvironment(_environment.get());

    _intermediateImage = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            colorImageFormat,
            initialExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        )
    );
}

void RendererDeferred::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
) {
    _environment->initFrameResources(frameAllocator);
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
}

void RendererDeferred::resize(
    VkExtent2D newExtent
) {
    _viewportExtent = newExtent;

    _skyboxLayer->resize(newExtent);

    _intermediateImage = std::unique_ptr<vulkan::Image>(
        vulkan::Image::createAllocatedImage(
            _engine,
            _colorImageFormat,
            newExtent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        )
    );

    _scene->willResize();
    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);
    _scene->didResize();
}

void RendererDeferred::update(
    float delta
) {
    updateScene(delta);
}

void RendererDeferred::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* colorImage,
    const bg2e::render::vulkan::Image* /*depthImage*/,
    const bg2e::render::vulkan::Image* /*msaaDepthImage*/,
    bg2e::render::vulkan::FrameResources& frameResources
) {
    prepareSceneRender(cmd, currentFrame, frameResources);

    _skyboxLayer->render(cmd, currentFrame, nullptr, _intermediateImage.get(), frameResources);

    vulkan::Image::cmdTransitionImage(
        cmd,
        _intermediateImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    vulkan::Image::cmdTransitionImage(
        cmd,
        colorImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    vulkan::Image::cmdCopy(
        cmd,
        _intermediateImage->handle(),
        _intermediateImage->extent2D(),
        colorImage->handle(),
        colorImage->extent2D()
    );
    vulkan::Image::cmdTransitionImage(
        cmd,
        colorImage->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    endSceneRender();
}

void RendererDeferred::cleanup() {
    _intermediateImage.reset();
    _skyboxLayer.reset();
    _scene.reset();
}

}
