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
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/deferred/TemporalAccumulator.hpp>
#include <bg2e/render/deferred/DenoiseFilter.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/scene/vk/EnvironmentDataBinding.hpp>
#include <bg2e/scene/vk/DeferredLightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>
#include <bg2e/render/RenderQueue.hpp>

namespace bg2e {
namespace render {
namespace deferred {

enum class LayerType {
    Opaque,
    Transparent
};

enum class DeferredDebugVisualization {
    FullComposition = 0,
    GBufferAlbedo,
    GBufferNormal,
    GBufferMaterial,
    GBufferFresnelFlags,
    GBufferSheenColor,
    GBufferDepth,
    InputImage,
    RTAmbientOcclusion,
    DenoisedAO,
    TemporalAccumulatedAO,

    MaxLayer
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

    void setLightDataBinding(scene::vk::DeferredLightDataBinding* binding) { _lightDataBinding = binding; }
    void setLights(const std::vector<base::LightData>& l) { _lights = l; }
    void setRtDataBinding(vulkan::rt::RayTracingSceneDataBinding* rt) { _rtDataBinding = rt; }
    void setRenderQueue(render::RenderQueue<scene::Drawable>* rq) { _renderQueue = rq; }

    void setDebugVisualization(DeferredDebugVisualization mode) { _debugVisualization = mode; }
    DeferredDebugVisualization debugVisualization() const { return _debugVisualization; }

    void setIsTransparent(bool isTransparent) { _isTransparent = isTransparent; }
    [[nodiscard]] bool isTransparent() const { return _isTransparent; }
    void setOpaqueDepthBuffer(std::shared_ptr<vulkan::Image> image) { _opaqueDepthBuffer = image; }
    std::shared_ptr<vulkan::Image> depthBuffer();

protected:
    LayerType _layerType;

    std::vector<std::unique_ptr<GBufferManager>> _gbuffers;
    std::shared_ptr<vulkan::Image> _opaqueDepthBuffer;

    VkPipeline _gbufferPipeline = VK_NULL_HANDLE;
    VkPipeline _compositePipeline = VK_NULL_HANDLE;
    VkPipeline _compositePipelineRT = VK_NULL_HANDLE;
    VkPipelineLayout _gbufferPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _compositePipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _compositePipelineRTLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout _compositeGBufferDSLayout = VK_NULL_HANDLE;

    std::unique_ptr<scene::vk::FrameDataBinding> _frameDataBinding;
    std::unique_ptr<scene::vk::FrameDataBinding> _fragmentFrameDataBinding;
    std::unique_ptr<scene::vk::ObjectDataBinding> _objectDataBinding;
    std::unique_ptr<scene::vk::EnvironmentDataBinding> _environmentDataBinding;
    scene::vk::DeferredLightDataBinding* _lightDataBinding = nullptr;
    std::vector<base::LightData> _lights;
    vulkan::rt::RayTracingSceneDataBinding* _rtDataBinding = nullptr;
    render::RenderQueue<scene::Drawable>* _renderQueue = nullptr;

    VkSampler _gbufferSampler = VK_NULL_HANDLE;
    bool _useRtShadows = false;

    DeferredDebugVisualization _debugVisualization = DeferredDebugVisualization::FullComposition;

    VkPipeline _debugPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _debugPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _debugDSLayout = VK_NULL_HANDLE;

    bool _isTransparent = false;

    std::unique_ptr<RTAmbientOcclusion> _rtAmbientOcclusion;
    std::unique_ptr<TemporalAccumulator> _temporalAccumulator;
    std::unique_ptr<DenoiseFilter> _denoiseFilter;

    struct CompositePushConstants {
        float gamma;
        float brightness;
        float contrast;
        float exposure;

        uint32_t numLights;

        uint32_t padding1;
        uint32_t padding2;
        uint32_t padding3;

        glm::mat4 inverseViewProjection;
    };

    void createGBufferPipeline();
    void createCompositePipeline();
    void createCompositePipelineRT();
    void createDebugPipeline();
    void renderGBufferPass(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        GBufferManager * gbuffer,
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
    void renderDebugPass(
        VkCommandBuffer cmd,
        const vulkan::Image* sourceImage,
        const vulkan::Image* outputImage,
        vulkan::FrameResources& frameResources
    );
    const vulkan::Image* resolveDebugSource(const vulkan::Image* inputImage, GBufferManager* gbuffer) const;
};

}
}
}
