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
#include <bg2e/scene/vk/DeferredLightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingSceneDataBinding.hpp>
#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>
#include <bg2e/manipulation/GizmoAndSelectionRenderer.hpp>
#include <bg2e/render/deferred/SMAAProcessor.hpp>

#include <memory>
#include <cmath>

namespace bg2e {
namespace render {

class BG2E_API RendererDeferred : public Renderer {
public:
    RendererDeferred() = default;
    ~RendererDeferred() override;

    [[nodiscard]] bool drawSkybox() const override { return _skyboxLayer->drawSkybox(); }
    inline void setDrawSkybox(bool value) override { _skyboxLayer->setDrawSkybox(value); }

    [[nodiscard]] manipulation::GizmoAndSelectionRenderer * gizmoAndSelectionRenderer() const { return _gizmoAndSelectionRenderer.get(); }
    [[nodiscard]] deferred::SMAAProcessor* smaaProcessor() const { return _smaaProcessor.get(); }

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

    // Render scale: percentage of viewport size used for deferred rendering.
    // 100% = native resolution, <100% = lower internal resolution (for upscaling),
    // >100% = supersampling. Range is clamped to [25, 150].
    void setRenderScalePercent(float percent);
    float renderScalePercent() const { return _renderScalePercent; }

    // Internal deferred render size (computed from viewport + render scale)
    uint32_t renderWidth() const { return _renderExtent.width; }
    uint32_t renderHeight() const { return _renderExtent.height; }

    deferred::DeferredDebugVisualization debugVisualization() const;
    void setDebugVisualization(deferred::DeferredDebugVisualization debugVisualization);

    void setAOQuality(deferred::RTAOQuality quality);
    deferred::RTAOQuality aoQuality() const;

    void setAOSampleCount(int count);
    int aoSampleCount() const;

    void setAOBounceCount(int count);
    int aoBounceCount() const;

    void setAORadius(float radius);
    float aoRadius() const;

    void setAOBias(float bias);
    float aoBias() const;

    void setAOFalloff(float falloff);
    float aoFalloff() const;

    void setAOBounceAttenuation(float attenuation);
    float aoBounceAttenuation() const;

    void setTemporalHistoryWeight(float weight);
    float temporalHistoryWeight() const;

    void setTemporalDepthThreshold(float threshold);
    float temporalDepthThreshold() const;

    void setTemporalNormalThreshold(float threshold);
    float temporalNormalThreshold() const;

    void setTemporalMode(deferred::TemporalAccumulator::AccumulationMode mode);
    deferred::TemporalAccumulator::AccumulationMode temporalMode() const;

    void setDenoiseKernelRadius(int radius);
    int denoiseKernelRadius() const;

    void setDenoiseDepthThreshold(float threshold);
    float denoiseDepthThreshold() const;

    void setDenoiseNormalThreshold(float threshold);
    float denoiseNormalThreshold() const;

    void setDenoiseDepthSigma(float sigma);
    float denoiseDepthSigma() const;

    void setDenoiseNormalSigma(float sigma);
    float denoiseNormalSigma() const;

    // Indirect lighting mode (RTAO vs RTGI)
    void setIndirectLightingMode(deferred::IndirectLightingMode mode);
    deferred::IndirectLightingMode indirectLightingMode() const;

    void setRTGIEnabled(bool enabled);
    bool rtGIEnabled() const;

    void setRTGISampleCount(uint32_t count);
    uint32_t rtGISampleCount() const;

    void setRTGIBounceCount(uint32_t count);
    uint32_t rtGIBounceCount() const;

    void setRTGIRayBias(float bias);
    float rtGIRayBias() const;

    void setRTGIMaxDistance(float distance);
    float rtGIMaxDistance() const;

    void setRTGIQuality(deferred::RTGIQuality quality);
    deferred::RTGIQuality rtGIQuality() const;

    // RT Reflections
    void setRTReflectionsEnabled(bool enabled);
    bool rtReflectionsEnabled() const;

    void setRTReflectionSampleCount(uint32_t count);
    uint32_t rtReflectionSampleCount() const;

    void setRTReflectionMaxRoughness(float roughness);
    float rtReflectionMaxRoughness() const;

    void setRTReflectionRayBias(float bias);
    float rtReflectionRayBias() const;

    void setRTReflectionMaxDistance(float distance);
    float rtReflectionMaxDistance() const;

    void setRTReflectionRoughnessSpread(float spread);
    float rtReflectionRoughnessSpread() const;

protected:
    bg2e::render::Engine* _engine = nullptr;

    VkExtent2D _viewportExtent;   // Final display/swapchain size
    VkExtent2D _renderExtent;     // Internal deferred render size (may differ from viewport)
    VkFormat _colorImageFormat;
    VkFormat _depthImageFormat;
    VkSampleCountFlagBits _sampleCount;

    float _renderScalePercent = 50.0f;

    // Layers
    std::unique_ptr<deferred::SkyboxLayer> _skyboxLayer;
    std::unique_ptr<deferred::DeferredLayer> _opaqueLayer;
    std::unique_ptr<deferred::DeferredLayer> _transparentLayer;

    // Intermediate images
    std::shared_ptr<vulkan::Image> _skyboxImage;
    std::shared_ptr<vulkan::Image> _opaqueImage;
    std::shared_ptr<vulkan::Image> _transparentImage;

    // Data bindings (shared across deferred layers)
    std::unique_ptr<scene::vk::DeferredLightDataBinding> _lightDataBinding;
    std::unique_ptr<vulkan::rt::RayTracingSceneDataBinding> _rtDataBinding;
    std::unique_ptr<vulkan::rt::ReflectionLightDataBinding> _reflectionLightDataBinding;

    std::unique_ptr<manipulation::GizmoAndSelectionRenderer> _gizmoAndSelectionRenderer;
    std::unique_ptr<deferred::SMAAProcessor> _smaaProcessor;

    deferred::DeferredDebugVisualization _debugVisualization = deferred::DeferredDebugVisualization::FullComposition;

    std::vector<base::LightData> _lights;
    std::vector<base::LightData> _reflectionLights;

protected:
    void updateLights(const std::vector<std::shared_ptr<bg2e::scene::LightComponent>>& lightComponents, uint32_t maxLights) override;

    VkExtent2D computeRenderExtent(VkExtent2D viewportExtent, float scalePercent) const;
};

}
}
