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

#include <bg2e/render/deferred/RenderLayer.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>
#include <bg2e/render/RenderQueue.hpp>

namespace bg2e {
namespace render {
namespace deferred {

enum class LayerType {
    Opaque,
    Transparent
};

class BG2E_API DeferredLayer : public RenderLayer {
public:
    DeferredLayer(Engine* engine, LayerType type);
    ~DeferredLayer() override;

    void build(VkExtent2D extent, VkFormat outputFormat) override;
    void initFrameResources(vulkan::DescriptorSetAllocator* allocator) override;
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vulkan::Image* inputImage,
        const vulkan::Image* outputImage,
        vulkan::FrameResources& frameResources
    ) override;
    void resize(VkExtent2D newExtent) override;
    void cleanup() override;

    void setLightDataBinding(scene::vk::LightDataBinding* binding) { _lightDataBinding = binding; }
    void setLightUniforms(const scene::vk::LightDataBinding::LightUniforms& lu) { _lightUniforms = lu; }
    void setRtDataBinding(vulkan::rt::RayTracingSceneDataBinding* rt) { _rtDataBinding = rt; }
    void setRenderQueue(render::RenderQueue<scene::Drawable>* rq) { _renderQueue = rq; }

protected:
    LayerType _layerType;

    std::unique_ptr<GBufferManager> _gbuffer;

    VkPipeline _gbufferPipeline = VK_NULL_HANDLE;
    VkPipeline _compositePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _gbufferPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _compositePipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout _gbufferFrameDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _gbufferObjectDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _gbufferEnvDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _gbufferLightDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _gbufferRtDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _compositeGBufferDSLayout = VK_NULL_HANDLE;

    std::unique_ptr<scene::vk::FrameDataBinding> _frameDataBinding;
    std::unique_ptr<scene::vk::ObjectDataBinding> _objectDataBinding;
    std::unique_ptr<scene::vk::EnvironmentDataBinding> _environmentDataBinding;
    scene::vk::LightDataBinding* _lightDataBinding = nullptr;
    scene::vk::LightDataBinding::LightUniforms _lightUniforms;
    vulkan::rt::RayTracingSceneDataBinding* _rtDataBinding = nullptr;
    render::RenderQueue<scene::Drawable>* _renderQueue = nullptr;

    VkSampler _gbufferSampler = VK_NULL_HANDLE;
    bool _useRtShadows = false;

    struct CompositePushConstants {
        float gamma;
        float brightness;
        float contrast;
        float exposure;
    };

    void createGBufferPipeline();
    void createCompositePipeline();
    void renderGBufferPass(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec3& cameraWorldPos
    );
    void renderCompositePass(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vulkan::Image* inputImage,
        const vulkan::Image* outputImage,
        vulkan::FrameResources& frameResources,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix
    );
};

}
}
}
