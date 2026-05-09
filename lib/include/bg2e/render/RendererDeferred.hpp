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

#include <memory>

namespace bg2e {
namespace render {

class BG2E_API RendererDeferred : public Renderer {
public:
    RendererDeferred() = default;
    ~RendererDeferred() override = default;

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
        bg2e::render::vulkan::FrameResources& frameResources
    ) override;

    void cleanup() override;

    bg2e::scene::Scene* scene() override;

    void setBrightness(float b) override { _brightness = b; }
    [[nodiscard]] float brightness() const override { return _brightness; }

    void setContrast(float c) override { _contrast = c; }
    [[nodiscard]] float contrast() const override { return _contrast; }

    void setExposure(float e) override { _exposure = e; }
    [[nodiscard]] float exposure() const override { return _exposure; }

    uint32_t viewportWidth() { return _viewportExtent.width; }
    uint32_t viewportHeight() { return _viewportExtent.height; }

protected:
    bg2e::render::Engine* _engine = nullptr;

    VkExtent2D _viewportExtent;
    VkFormat _colorImageFormat;
    VkFormat _depthImageFormat;
    VkSampleCountFlagBits _sampleCount;

    bool _isOffscreen;

    std::unique_ptr<bg2e::scene::Scene> _scene;

    float _brightness = 0.0f;
    float _contrast = 1.0f;
    float _exposure = 1.0f;
};

}
}
