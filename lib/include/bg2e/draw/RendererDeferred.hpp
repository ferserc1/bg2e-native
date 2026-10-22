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
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>
#include <bg2e/draw/pipeline/RTReflections.hpp>
#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/draw/pipeline/TemporalSurfacePass.hpp>
#include <bg2e/draw/pipeline/DenoiseFilter.hpp>
#include <bg2e/draw/pipeline/FinalProcessor.hpp>
#include <bg2e/draw/pipeline/DeferredLayer.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/draw/environment/SkyboxRenderer.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/scene/Component.hpp>
#include <map>

namespace bg2e::draw {
class TemporalSignatureCache;
class TemporalSignatureState;
class TemporalCorrespondence;
struct TemporalCorrespondenceFrame;
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

    // Split recording: retain the returned borrowed-slot owner until completion.
    // recordFinal uses the same updated slot and accepts resolved linear RGBA16F.
    bool supportsLinearRecording() const override;
    std::shared_ptr<gpu::Image> recordLinear(const FrameContext& frame) override;
    void recordFinal(const FrameContext& frame, gpu::Image& linearColor) override;
    std::vector<uint8_t> sceneFingerprint() const override;

    // Explicit camera overrides first enabled scene camera; no camera uses identity.
    void setCamera(const glm::mat4& view, const glm::mat4& projection);
    void useSceneCamera();
    void resetUpscaler(); // Compatibility hook: resets both effect and SDK histories.
    void invalidateHistories() override;
    uint64_t historyGeneration() const;

    // Diagnostic baseline and exact comparison against full serialization.
    // Change controls at the normal synchronized renderer update boundary.
    void setTemporalSignatureReuseEnabled(bool enabled)
    {
        _signatureReuseEnabled = enabled;
    }
    void setTemporalSignatureValidationEnabled(bool enabled)
    {
        _signatureValidationEnabled = enabled;
    }

    std::shared_ptr<pipeline::TemporalAccumulator> temporal() const;
    std::shared_ptr<pipeline::DenoiseFilter> denoise() const;

    // Experimental receiver replay. Opt in at a stopped update boundary; default
    // stays off until native validation.
    bool temporalSurfaceEnabled() const;
    void setTemporalSurfaceEnabled(bool value);
    bool temporalSurfaceAvailable() const;
    const std::string& temporalSurfaceStatus() const;
    pipeline::TemporalDiagnosticView temporalDiagnosticView() const;
    void setTemporalDiagnosticView(pipeline::TemporalDiagnosticView value);
    pipeline::TemporalSignal temporalDiagnosticSignal() const;
    void setTemporalDiagnosticSignal(pipeline::TemporalSignal value);
    uint64_t temporalDiagnosticLayer() const;
    void setTemporalDiagnosticLayer(uint64_t value); // 0 opaque; exact transparent TemporalKey layer otherwise.
    bool temporalDiagnosticsAvailable() const;
    // Snapshot only at a synchronized frame boundary. Owners remain valid across slot reuse.
    pipeline::TemporalSurfaceMetadata temporalSurfaceMetadata(uint32_t slot, uint64_t layer = 0) const;

    std::shared_ptr<pipeline::FinalProcessor> finalProcessor() const;
    const std::string& noiseStatus() const;
    std::shared_ptr<Upscaler> upscaler() const;
    std::shared_ptr<ResolutionSettings> resolution() const;
    void setCameraParameters(float nearPlane, float farPlane, float verticalFovRadians);

    pipeline::FinalSettings& finalSettings();
    const RenderQueue& queue() const;
    // Diagnostics only; change reuse flags at a synchronized update boundary.
    std::shared_ptr<pipeline::TracingSceneResources> tracingResources() const;
    std::shared_ptr<pipeline::SoftShadowPass> softShadows() const;
    bool usesQueryShadows() const;
    std::shared_ptr<pipeline::RTAmbientOcclusion> ambientOcclusion() const;
    bool usesAmbientOcclusion() const;
    bool ambientOcclusionEnabled() const;
    std::shared_ptr<pipeline::RTGlobalIllumination> globalIllumination() const;
    bool usesGlobalIllumination() const;
    bool globalIlluminationEnabled() const;
    std::shared_ptr<pipeline::RTReflections> reflections() const;
    bool usesReflections() const;
    bool reflectionsEnabled() const;
    IndirectLightingMode indirectLightingMode() const;
    void setIndirectLightingMode(IndirectLightingMode value);
    bool inspectOpaqueLayer() const;
    void setInspectOpaqueLayer(bool value);
    environment::EnvironmentGenerator* environmentGenerator() const;
    const std::string& environmentError() const;
    // Recoverable initialization failures and the selected fallback, also
    // exposed by the graphics diagnostics catalog. Empty on a clean startup.
    const std::string& initializationWarnings() const;
    // Diagnostic baseline for transparent rasterization and composition.
    void setTransparentRegionsEnabled(bool enabled)
    {
        _transparentRegionsEnabled = enabled;
    }

  private:
    struct Targets {
        std::vector<std::shared_ptr<gpu::Image>> gbuffer;
        std::shared_ptr<gpu::Image> opaqueDepth, colorA, colorB, reconstructed;
        UpscalerCamera camera;
        glm::mat4 jitteredProjection{1};
        std::vector<std::shared_ptr<gpu::Image>> shadows;
        std::shared_ptr<gpu::Image> ao, gi, reflections;
        RenderQueue queue;
        std::shared_ptr<pipeline::TracingSnapshot> tracing;
        std::shared_ptr<const TemporalCorrespondenceFrame> preparedCorrespondence, recordedCorrespondence;
        pipeline::TemporalSurfaceResult opaqueTemporalSurface;
        std::map<uint64_t,pipeline::TemporalSurfaceResult> transparentTemporalSurfaces;
        uint64_t serial = 0;
        bool prepared = false, linearRecorded = false, finalRecorded = false;
        bool sceneMotion = false; // Accepted transform observation; consumed by temporal scheduling.
        pipeline::TemporalDiagnosticView diagnosticView = pipeline::TemporalDiagnosticView::Off;
        pipeline::TemporalSignal diagnosticSignal = pipeline::TemporalSignal::Visibility;
        uint64_t diagnosticLayer = 0;
        std::shared_ptr<gpu::Image> diagnostic;
        bool recordTemporalSurfaces = false; // Requested/capable state latched for this scene update.
        const RenderQueue::Item* finalItem = nullptr;
    };

    void buildTargets();
    void warnInitialization(const std::string& message);
    void updateTemporalSurfaceStatus();
    std::vector<uint8_t> fullTemporalSignature(const RenderQueue& queue) const;
    std::vector<uint8_t> historyPolicySignature(const RenderQueue& queue) const;
    std::vector<uint8_t> serializeTemporalSignature(const RenderQueue& queue, bool includeTransforms) const;
    std::shared_ptr<const TemporalSignatureState> prepareTemporalSignature(const RenderQueue& queue) const;
    void resetHistories();
    UpscalerConfiguration upscaleConfiguration() const;
    pipeline::OverlayConfiguration overlayConfiguration() const;

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
    std::shared_ptr<pipeline::TracingSceneResources> _tracingResources;
    bool _ambientOcclusion = false;
    bool _globalIllumination = false;
    bool _reflections = false;
    uint64_t _historyGeneration = 1;
    std::vector<uint8_t> _historySignature; // Full baseline only while diagnostics are active.
    std::vector<uint8_t> _historyPolicySignature; // Reduced reference baseline; never full fingerprint bytes.
    bool _historySignatureCurrent = false;
    bool _transparentRegionsEnabled = true;
    bool _signatureReuseEnabled = true, _signatureValidationEnabled = false;
    mutable std::shared_ptr<TemporalSignatureCache> _signatureCache;
    std::shared_ptr<const TemporalSignatureState> _historyState;
    std::shared_ptr<TemporalCorrespondence> _temporalCorrespondence;
    std::unique_ptr<pipeline::TemporalSurfacePass> _temporalSurface;
    bool _temporalSurfaceEnabled = false;
    pipeline::TemporalDiagnosticView _temporalDiagnosticView = pipeline::TemporalDiagnosticView::Off;
    pipeline::TemporalSignal _temporalDiagnosticSignal = pipeline::TemporalSignal::Visibility;
    uint64_t _temporalDiagnosticLayer = 0;
    std::string _temporalSurfaceStatus = "Not initialized";
    std::shared_ptr<scene::Node> _scene;
    std::unique_ptr<environment::EnvironmentGenerator> _generator;
    std::shared_ptr<const environment::EnvironmentResources> _environment;
    std::shared_ptr<gpu::PipelineLayout> _drawLayout, _lightingLayout;
    FrameDataBinding _frame;
    ObjectDataBinding _object;
    LightDataBinding _lights;
    EnvironmentDataBinding _environmentBinding;
    std::vector<Targets> _targets;
    size_t _initializedOverlays = 0;
    RenderQueue _queue;
    glm::mat4 _view{1}, _projection{1};
    bool _explicitCamera = false;
    const void* _temporalCameraIdentity = nullptr;
    std::shared_ptr<scene::Component> _temporalCameraOwner;
    bool _inspectOpaqueLayer = false;
    std::string _environmentError;
    std::string _initializationWarnings;
    std::shared_ptr<utils::BlueNoise> _sharedNoise;
    std::string _noiseStatus = "No shared atlas allocated; effects use injected noise or hash";
    std::shared_ptr<ResamplingUpscaler> _fallback;
    UpscalerCamera _upscaleCamera;
};
}
