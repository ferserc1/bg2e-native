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

#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/RenderQueue.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/DrawVisitor.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/RenderQueueVisitor.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/Renderer.hpp>
#include <bg2e/manipulation/SelectionHighlight.hpp>
#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>

#include <memory>

namespace bg2e {
namespace render {

// TODO: Refactor this class to be more generic, and implement a specific renderer for each rendering technique (forward, deferred, etc.)
class BG2E_API RendererBasicForward : public Renderer {
public:
    RendererBasicForward() = default;
    ~RendererBasicForward() override = default;

    [[nodiscard]] VkPipeline opaquePipeline() const { return _opaquePipeline; }
    [[nodiscard]] VkPipeline transparentPipeline() const { return _transparentPipeline; }
    [[nodiscard]] VkPipelineLayout pipelineLayout() const { return _pipelineLayout; }

    [[nodiscard]] const bg2e::scene::vk::FrameDataBinding* frameDataBinding() const { return _frameDataBinding.get(); }
    [[nodiscard]] const bg2e::scene::vk::ObjectDataBinding* objectDataBinding() const { return _objectDataBinding.get(); }
    [[nodiscard]] const bg2e::scene::vk::EnvironmentDataBinding* environmentDataBinding() const { return _environmentDataBinding.get(); }
    [[nodiscard]] const bg2e::scene::vk::LightDataBinding* lightDataBinding() const { return _lightDataBinding.get(); }
    [[nodiscard]] const bg2e::render::vulkan::rt::RayTracingSceneDataBinding* rtDataBinding() const { return _rtDataBinding.get(); }

    inline bg2e::scene::vk::FrameDataBinding* frameDataBinding() { return _frameDataBinding.get(); }
    inline bg2e::scene::vk::ObjectDataBinding* objectDataBinding() { return _objectDataBinding.get(); }
    inline bg2e::scene::vk::EnvironmentDataBinding* environmentDataBinding() { return _environmentDataBinding.get(); }
    inline bg2e::scene::vk::LightDataBinding* lightDataBinding() { return _lightDataBinding.get(); }
    inline bg2e::render::vulkan::rt::RayTracingSceneDataBinding* rtDataBinding() { return _rtDataBinding.get(); }

    [[nodiscard]] bool drawSkybox() const override { return _drawSkybox; }
    inline void setDrawSkybox(bool value) override { _drawSkybox = value; }

    [[nodiscard]] manipulation::SelectionHighlight * selectionHighlight() const { return _selectionHighlight.get(); }

    // This renderer is a prototype for a simple forward renderer. In the future, it will be possible to implement other rendering techniques by inheriting from this class and overriding the necessary methods.
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

    void createPipelines(bg2e::render::Engine* engine);

    uint32_t viewportWidth() override { return _engine->swapchain().extent().width; }
    uint32_t viewportHeight() override { return _engine->swapchain().extent().height; }

    bool supportsMsaa() override { return true; }

protected:

    bg2e::render::Engine* _engine = nullptr;

    std::unique_ptr<bg2e::scene::vk::FrameDataBinding> _frameDataBinding;
    std::unique_ptr<bg2e::scene::vk::ObjectDataBinding> _objectDataBinding;
    std::unique_ptr<bg2e::scene::vk::EnvironmentDataBinding> _environmentDataBinding;
    std::unique_ptr<bg2e::scene::vk::LightDataBinding> _lightDataBinding;
    std::unique_ptr<bg2e::render::vulkan::rt::RayTracingSceneDataBinding> _rtDataBinding;

    bg2e::scene::DrawVisitor _drawVisitor;

    std::unique_ptr<bg2e::manipulation::SelectionHighlight> _selectionHighlight;

    VkPipeline _opaquePipeline = VK_NULL_HANDLE;
    VkPipeline _transparentPipeline = VK_NULL_HANDLE;
    VkPipeline _solidTransparentPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    bool _drawSkybox = true;

    scene::vk::LightDataBinding::LightUniforms _lightUniforms;

protected:
    void updateLights(const std::vector<std::shared_ptr<bg2e::scene::LightComponent>>& lightComponents, uint32_t maxLights) override;

    struct PushConstants {
        float gamma;
        float brightness;
        float contrast;
        float exposure;
    };

    VkPipeline createOpaquePipeline(bg2e::render::Engine * engine, VkPipelineLayout layout);
    VkPipeline createTransparentPipeline(bg2e::render::Engine * engine, VkPipelineLayout layout);
    VkPipeline createSolidTransparentPipeline(bg2e::render::Engine * engine, VkPipelineLayout layout);
};

}
}
