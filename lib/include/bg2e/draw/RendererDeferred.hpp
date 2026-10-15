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

#include <bg2e/draw/Renderer.hpp>
#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <bg2e/draw/pipeline/SoftShadowPass.hpp>
#include <bg2e/draw/pipeline/DeferredLayer.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/draw/environment/SkyboxRenderer.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>

namespace bg2e::draw {
// Ordered single-sample deferred graph. Configuration.layers replaces matching
// gbuffer/deferred/final/skybox contracts; missing stages get public defaults.
// All lifecycle and prepareFrame calls require stopped background producers.
class BG2E_API RendererDeferred : public Renderer {
  public:
    explicit RendererDeferred(std::filesystem::path shaderRoot = {}, uint32_t frameSlots = 0);
    const RendererConfiguration& configuration() const override;
    void initialize(Engine& engine, RendererConfiguration configuration) override;
    void attachScene(std::shared_ptr<scene::Node> sceneRoot) override;
    void resize(gpu::Size2D outputExtent, gpu::Size2D internalExtent) override;
    bool prepareFrame() override;
    bool needsReconfiguration() const override;
    void update(const FrameContext& frame) override;
    void record(const FrameContext& frame) override;
    void cleanup() override;
    // Explicit camera overrides first enabled scene camera; no camera uses identity.
    void setCamera(const glm::mat4& view, const glm::mat4& projection);
    void useSceneCamera();
    void resetUpscaler(); // Call at a stopped boundary after camera cuts/discontinuous edits.
    std::shared_ptr<Upscaler> upscaler() const;
    std::shared_ptr<ResolutionSettings> resolution() const;
    void setCameraParameters(float nearPlane, float farPlane, float verticalFovRadians);
    pipeline::FinalSettings& finalSettings();
    const RenderQueue& queue() const;
    std::shared_ptr<pipeline::SoftShadowPass> softShadows() const;
    bool usesQueryShadows() const;
    bool inspectOpaqueLayer() const;
    void setInspectOpaqueLayer(bool value);
    environment::EnvironmentGenerator* environmentGenerator() const;
    const std::string& environmentError() const;
  private:
    struct Targets {
        std::vector<std::shared_ptr<gpu::Image>> gbuffer;
        std::shared_ptr<gpu::Image> opaqueDepth, colorA, colorB, reconstructed;
        UpscalerCamera camera;
        glm::mat4 jitteredProjection{1};
        std::vector<std::shared_ptr<gpu::Image>> shadows;
        RenderQueue queue;
        uint64_t serial = 0;
        bool prepared = false;
    };
    void buildTargets();
    UpscalerConfiguration upscaleConfiguration() const;
    void checkFrame(const FrameContext& frame) const;
    void dispatch(size_t stage, const FrameContext& frame,
        const std::vector<pipeline::ImageBinding>& inputs,
        const std::vector<pipeline::ImageBinding>& outputs, const pipeline::RasterFrame* raster = nullptr);
    std::filesystem::path _shaderRoot;
    uint32_t _requestedSlots;
    Engine* _engine = nullptr;
    RendererConfiguration _configuration;
    pipeline::LayerConfiguration _shadowConfiguration;
    bool _queryShadows = false;
    std::shared_ptr<scene::Node> _scene;
    std::unique_ptr<environment::EnvironmentGenerator> _generator;
    std::shared_ptr<const environment::EnvironmentResources> _environment;
    std::shared_ptr<gpu::PipelineLayout> _drawLayout, _lightingLayout;
    FrameDataBinding _frame;
    ObjectDataBinding _object;
    LightDataBinding _lights;
    EnvironmentDataBinding _environmentBinding;
    std::vector<Targets> _targets;
    RenderQueue _queue;
    glm::mat4 _view{1}, _projection{1};
    bool _explicitCamera = false;
    bool _inspectOpaqueLayer = false;
    std::string _environmentError;
    std::shared_ptr<ResamplingUpscaler> _fallback;
    UpscalerCamera _upscaleCamera;
};
}
