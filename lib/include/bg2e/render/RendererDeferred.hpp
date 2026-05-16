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

#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/Renderer.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/deferred/SkyboxLayer.hpp>
#include <bg2e/render/deferred/DeferredLayer.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>
#include <bg2e/manipulation/SelectionHighlight.hpp>

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API RendererDeferred : public Renderer {
public:
    RendererDeferred() = default;
    ~RendererDeferred() override;

    [[nodiscard]] bool drawSkybox() const override { return _skyboxLayer->drawSkybox(); }
    inline void setDrawSkybox(bool value) override { _skyboxLayer->setDrawSkybox(value); }

    void build(
        bg2e::render::Engine* engine,
        VkExtent2D initialExtent,
        VkFormat colorImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits sampleCount,
        bool isOffscreen
    ) override;

    void initFrameResources(
        bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
    ) override;

    void initScene(
        std::shared_ptr<bg2e::scene::Node> sceneRoot
    ) override;

    void resize(
        VkExtent2D newExtent
    ) override;

    void update(
        float delta
    ) override;

    void draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* colorImage,
        const bg2e::render::vulkan::Image* depthImage,
        const bg2e::render::vulkan::Image* msaaDepthImage,
        bg2e::render::vulkan::FrameResources& frameResources,
        VkImageLayout & outColorImageLayout,
        VkImageLayout & outDepthImageLayout,
        VkImageLayout & outMsaaDepthImageLayout
    ) override;

    void cleanup() override;

    uint32_t viewportWidth() override { return _viewportExtent.width; }
    uint32_t viewportHeight() override { return _viewportExtent.height; }

    bool supportsMsaa() override { return false; }

protected:
    bg2e::render::Engine* _engine = nullptr;

    VkExtent2D _viewportExtent;
    VkFormat _colorImageFormat;
    VkFormat _depthImageFormat;
    VkSampleCountFlagBits _sampleCount;

    // Layers
    std::unique_ptr<deferred::SkyboxLayer> _skyboxLayer;
    std::unique_ptr<deferred::DeferredLayer> _opaqueLayer;
    std::unique_ptr<deferred::DeferredLayer> _transparentLayer;

    // Intermediate images
    std::shared_ptr<vulkan::Image> _skyboxImage;
    std::shared_ptr<vulkan::Image> _opaqueImage;

    // Data bindings (shared across deferred layers)
    std::unique_ptr<scene::vk::LightDataBinding> _lightDataBinding;
    std::unique_ptr<vulkan::rt::RayTracingSceneDataBinding> _rtDataBinding;

    // Selection highlight
    std::unique_ptr<manipulation::SelectionHighlight> _selectionHighlight;
};

}
}
