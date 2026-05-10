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

#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/RenderQueueVisitor.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>

namespace bg2e {
namespace render {

class BG2E_API Renderer {
public:
    virtual ~Renderer() = default;

    virtual void build(
        bg2e::render::Engine* engine,
        VkExtent2D initialExtent,
        VkFormat colorImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits sampleCount,
        bool isOffscreen
    ) = 0;
    
    virtual void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) = 0;
    virtual void initScene(std::shared_ptr<bg2e::scene::Node> sceneRoot) = 0;
    virtual void resize(VkExtent2D newExtent) = 0;
    virtual void update(float delta) = 0;
    virtual void draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* colorImage,
        const bg2e::render::vulkan::Image* depthImage,
        const bg2e::render::vulkan::Image* msaaDepthImage,
        bg2e::render::vulkan::FrameResources& frameResources
    ) = 0;
    virtual void cleanup() = 0;

    bg2e::scene::Scene* scene() const { return _scene.get(); };

    virtual void setDrawSkybox(bool drawSkybox) = 0;
    virtual bool drawSkybox() const = 0;
    int skyboxBlurLevel() const { return _environment->skyboxBlurLevel(); }
    void setSkyboxBlurLevel(int l) { _environment->setSkyboxBlurLevel(l); }
    
    virtual uint32_t viewportWidth() = 0;
    virtual uint32_t viewportHeight() = 0;

    virtual bool supportsMsaa() = 0;

    void setBrightness(float b) { _brightness = b; }
    [[nodiscard]] float brightness() const { return _brightness; }

    void setContrast(float c) { _contrast = c; }
    [[nodiscard]] float contrast() const { return _contrast; }

    void setExposure(float e) { _exposure = e; }
    [[nodiscard]] float exposure() const { return _exposure; }

protected:
    std::unique_ptr<EnvironmentResources> _environment;
    size_t _skyImageHash;

    VkExtent2D _viewportExtent;
    VkFormat _colorImageFormat;
    VkFormat _depthImageFormat;
    VkSampleCountFlagBits _sampleCount;

    bool _isOffscreen;

    bg2e::scene::ResizeViewportVisitor _resizeVisitor;
    bg2e::scene::UpdateVisitor _updateVisitor;
    bg2e::scene::RenderQueueVisitor<bg2e::scene::Drawable> _renderQueueVisitor;

    bg2e::render::RenderQueue<bg2e::scene::Drawable> _renderQueue;

    std::unique_ptr<bg2e::scene::Scene> _scene;

    float _brightness = 0.0f;
    float _contrast = 1.0f;
    float _exposure = 1.0f;

    scene::vk::LightDataBinding::LightUniforms _lightUniforms;

    // Call the update callbacks in the scene, update the environment image
    // and generate the light data
    void updateScene(float delta, uint32_t maxLights = std::numeric_limits<uint32_t>::max());

    // Call the begin render scene callbacks, update the skybox image if needed,
    // enqueue the drawable objects in render queue
    void prepareSceneRender(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        render::vulkan::FrameResources& frameResources
    );

    // Call the end renderer scene callbacks
    void endSceneRender();
};

}
}
