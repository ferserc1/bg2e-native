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

#include <bg2e/draw/RendererDeferred.hpp>
#include "TemporalSignatureCache.hpp"
#include "TemporalCorrespondence.hpp"
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/draw/pipeline/ProcessingRegion.hpp>
#include <typeinfo>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <cstddef>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>

namespace bg2e::draw {
namespace {
class SceneInputs : public scene::NodeVisitor {
  public:
    explicit SceneInputs(scene::Node* root)
    {
        setIgnoreDisabled(true);
        auto* owner = root ? root->sceneRoot()->scene() : nullptr;
        _explicitEnvironment = owner && owner->hasExplicitMainEnvironment();
        if (_explicitEnvironment)
        {
            environment = owner->mainEnvironment();
        }
    }

    scene::CameraComponent* camera = nullptr;
    scene::EnvironmentComponent* environment = nullptr;

    void visit(scene::Node* node) override
    {
        if (!camera)
        {
            camera = node->camera();
        }
        if (!_explicitEnvironment && !environment)
        {
            environment = node->getComponent<scene::EnvironmentComponent>();
        }
    }

  private:
    bool _explicitEnvironment = false;
};
}

struct RendererDeferred::GIUpsamplingState {
    struct alignas(16) Uniforms {
        glm::mat4 inverseViewProjection{1}, worldToView{1};
        glm::uvec4 extent{0};
        // Initial scene-scale quality choices, independent of tracing/denoise settings.
        glm::vec4 thresholds{0.01f, 0.01f, 0.9f, 1.0f};
        glm::vec4 parameters{0.5f, 0.1f, 1e-8f, 0.0f};
    };
    static_assert(sizeof(Uniforms) == 176);
    static_assert(offsetof(Uniforms, extent) == 128);
    static_assert(offsetof(Uniforms, thresholds) == 144);
    static_assert(offsetof(Uniforms, parameters) == 160);

    struct Invocation {
        uint64_t layer = 0;
        std::shared_ptr<gpu::Image> signal, normal, depth, output;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> resources;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::vector<Invocation> invocations;
    };

    std::shared_ptr<gpu::PipelineLayout> layout;
    std::shared_ptr<gpu::ShaderModule> shader;
    std::shared_ptr<gpu::Sampler> nearest;
    std::shared_ptr<gpu::ComputePipeline> pipeline;
    std::vector<Slot> slots;
};

bool RendererDeferred::giUpsamplingCompatible() const
{
    // Format/semantic compatibility alone does not establish a custom receiver mapping.
    return _globalIllumination && !_configuration.layers.empty() &&
        _configuration.layers[0].layer && _configuration.globalIllumination &&
        typeid(*_configuration.layers[0].layer) == typeid(pipeline::GBuffer) &&
        typeid(*_configuration.globalIllumination) == typeid(pipeline::RTGlobalIllumination);
}

void RendererDeferred::initializeGIUpsampling()
{
    if (!_engine || _giUpsampling || _targets.empty())
    {
        throw std::logic_error("GI upsampling initialization requires a clean renderer lifecycle and slots");
    }
    auto& device = *_engine->device();
    if (!device.formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(
        gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage))
    {
        throw std::runtime_error("GI upsampling requires sampled/storage RGBA16F support");
    }
    // Publish only a complete state; partial initialization releases local owners.
    auto state = std::make_shared<GIUpsamplingState>();
    gpu::PipelineLayoutDescription description;
    description.resourceBindings = {
        {0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute},
        {0,{1,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
        {0,{2,1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
        {0,{3,2},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute},
        {0,{4,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute},
        {0,{5,3},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute}
    };
    state->layout = device.createPipelineLayout(description);
    gpu::SamplerDescription nearest;
    nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
    nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
    nearest.maxLod = 0;
    state->nearest = device.createSampler(nearest);
    gpu::ShaderLib library(_shaderRoot, _engine->backendType());
    state->shader = library.compute("draw/gi_upsampling/joint_bilateral", &device);
    state->pipeline = device.createComputePipeline({state->shader.get(), state->layout.get()});
    state->slots.resize(_targets.size());
    _giUpsampling = std::move(state);
}

void RendererDeferred::resetGIUpsamplingSlots()
{
    // Only scene attachment/resize may discard every slot, after their device drain.
    if (_giUpsampling)
    {
        for (auto& slot : _giUpsampling->slots)
        {
            slot = {};
        }
    }
}

void RendererDeferred::retireGIUpsamplingSlot(uint32_t index, uint64_t serial)
{
    if (!_giUpsampling)
    {
        return;
    }
    auto& slot = _giUpsampling->slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Retire the acquired GI upsampling slot only once per serial");
    }
    slot = {};
    slot.serial = serial;
    slot.prepared = true;
}

void RendererDeferred::cleanupGIUpsampling()
{
    // Caller owns the existing renderer lifecycle drain; never wait while recording.
    _giUpsampling.reset();
}

std::shared_ptr<gpu::Image> RendererDeferred::recordGIUpsampling(gpu::CommandBuffer& command,
    const std::shared_ptr<gpu::Image>& signal, const std::shared_ptr<gpu::Image>& normal,
    const std::shared_ptr<gpu::Image>& depth, const glm::mat4& inverseViewProjection,
    const glm::mat4& worldToView, const std::shared_ptr<gpu::Image>& output,
    uint32_t index, uint64_t serial, uint64_t layer)
{
    if (!_giUpsampling || !signal || !output || signal->size() == output->size())
    {
        return signal;
    }
    auto& state = *_giUpsampling;
    auto& slot = state.slots.at(index);
    if (command.hasActiveScope() || !slot.prepared || slot.serial != serial ||
        (layer != 0 && layer != TransparentTemporalLayer) ||
        std::any_of(slot.invocations.begin(), slot.invocations.end(), [layer](const auto& invocation)
        {
            return invocation.layer == layer;
        }))
    {
        throw std::logic_error("GI upsampling requires a retired slot and a unique opaque/transparent invocation outside scopes");
    }
    auto* device = _engine->device();
    auto sampled = [device](const auto& image)
    {
        return image && image->isValid() && image->device() == device &&
            image->imageType() == gpu::ImageType::Image2D && !image->size().isZero() &&
            gpu::hasFlag(image->usage(), gpu::ImageUsage::Sampled);
    };
    if (!sampled(signal) || !sampled(normal) || !sampled(depth) ||
        !output->isValid() || output->device() != device || output->size().isZero() ||
        output->imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(output->usage(), gpu::ImageUsage::Sampled) ||
        !gpu::hasFlag(output->usage(), gpu::ImageUsage::Storage) ||
        output == signal || output == normal || output == depth ||
        signal->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
        output->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
        normal->size() != output->size() || depth->size() != output->size() ||
        (normal->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM &&
         normal->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT) ||
        (depth->pixelFormat() != gpu::PixelFormat::D16_UNORM && depth->pixelFormat() != gpu::PixelFormat::D32_SFLOAT))
    {
        throw std::invalid_argument("GI upsampling requires sampled camera guides and distinct internal-resolution RGBA16F storage");
    }
    GIUpsamplingState::Uniforms data;
    data.inverseViewProjection = inverseViewProjection;
    data.worldToView = worldToView;
    data.extent = {output->width(), output->height(), signal->width(), signal->height()};
    GIUpsamplingState::Invocation invocation;
    invocation.layer = layer;
    invocation.signal = signal;
    invocation.normal = normal;
    invocation.depth = depth;
    invocation.output = output;
    // Immutable per-invocation objects: no descriptors or uniforms are reused before retirement.
    invocation.uniforms = device->createBuffer("draw GI upsampling invocation uniforms");
    invocation.uniforms->createUniformBuffer(data);
    invocation.resources = device->createResourceSet(state.layout.get(), 0);
    invocation.resources->setUniformBuffer({0,1}, invocation.uniforms.get());
    invocation.resources->setSampledImage({1,0}, signal.get());
    invocation.resources->setSampledImage({2,1}, normal.get());
    invocation.resources->setSampledImage({3,2}, depth.get());
    invocation.resources->setSampler({4,0}, state.nearest.get());
    invocation.resources->setStorageImage({5,3}, output.get());
    invocation.resources->update();
    slot.invocations.push_back(std::move(invocation));
    command.transition(signal.get(), gpu::ImageLayout::ShaderReadOnly);
    command.transition(normal.get(), gpu::ImageLayout::ShaderReadOnly);
    command.transition(depth.get(), gpu::ImageLayout::ShaderReadOnly);
    command.transition(output.get(), gpu::ImageLayout::General);
    command.beginCompute();
    command.bindPipeline(state.pipeline.get());
    command.bindResourceSet(state.pipeline.get(), 0, slot.invocations.back().resources.get());
    command.dispatch((output->width()+15)/16, (output->height()+15)/16, 1);
    command.endCompute();
    command.transition(output.get(), gpu::ImageLayout::ShaderReadOnly);
    return output;
}

RendererDeferred::RendererDeferred(std::filesystem::path root, uint32_t slots)
    : _shaderRoot(root.empty() ? base::PlatformTools::shaderPath() : std::move(root)), _requestedSlots(slots)
{
}

const RendererConfiguration& RendererDeferred::configuration() const
{
    return _configuration;
}

void RendererDeferred::initialize(Engine& engine, RendererConfiguration configuration)
{
    if (_engine)
    {
        throw std::logic_error("Clean up RendererDeferred before initialization");
    }
    if (!configuration.resolution)
    {
        configuration.resolution = std::make_shared<ResolutionSettings>();
    }
    configuration.internalExtent = configuration.resolution->resolve(configuration.outputExtent, configuration.internalExtent);
    for (auto& layer : configuration.layers)
    {
        layer.configuration.internalExtent = configuration.internalExtent;
    }
    Renderer::validateConfiguration(configuration);
    if ((configuration.depthFormat != gpu::PixelFormat::D16_UNORM && configuration.depthFormat != gpu::PixelFormat::D32_SFLOAT) ||
        gpu::isUnsignedIntegerFormat(configuration.outputFormat) || gpu::isSignedIntegerFormat(configuration.outputFormat))
    {
        throw std::invalid_argument("Deferred renderer requires depth-only D16/D32 and a floating-point or normalized output");
    }
    _engine = &engine;
    _configuration = std::move(configuration);
    uint32_t count = _requestedSlots;
    if (count == 0)
    {
        count = engine.frameSlotCount();
    }
    if (!count)
    {
        throw std::invalid_argument("RendererDeferred requires acquired frame slots");
    }
    _targets.resize(count);
    if (!_configuration.upscaler)
    {
        _configuration.upscaler = std::make_shared<ResamplingUpscaler>();
    }
    if (_configuration.upscaler->identifier().empty() || _configuration.upscaler->label().empty())
    {
        throw std::invalid_argument("Upscalers require stable nonempty identifiers and labels");
    }
    _fallback = std::make_shared<ResamplingUpscaler>();
    _fallback->initialize(engine, upscaleConfiguration());
    try
    {
        _configuration.upscaler->initialize(engine, upscaleConfiguration());
    }
    catch (const std::runtime_error& error)
    {
        warnInitialization(std::string("Upscaler initialization failed; bilinear reconstruction active: ") + error.what());
        _configuration.upscaler->cleanup();
        _configuration.upscaler = std::make_shared<ResamplingUpscaler>();
        _configuration.upscaler->initialize(engine, upscaleConfiguration());
    }
    const auto depth = _configuration.depthFormat;
    if (!_configuration.softShadows)
    {
        _configuration.softShadows = std::make_shared<pipeline::SoftShadowPass>(_shaderRoot, count, depth);
    }
    _shadowConfiguration = {_configuration.outputExtent, _configuration.internalExtent, pipeline::softShadowContract(depth)};
    _configuration.softShadows->initialize(engine, _shadowConfiguration);
    if (!_configuration.ambientOcclusion)
    {
        pipeline::RTAOConfiguration aoConfiguration;
        aoConfiguration.shaderRoot = _shaderRoot;
        _configuration.ambientOcclusion = std::make_shared<pipeline::RTAmbientOcclusion>(aoConfiguration);
    }
    _configuration.ambientOcclusion->initialize(*engine.device(),engine.backendType(),count);
    if (!_configuration.globalIllumination)
    {
        pipeline::RTGIConfiguration giConfiguration;
        giConfiguration.shaderRoot = _shaderRoot;
        _configuration.globalIllumination = std::make_shared<pipeline::RTGlobalIllumination>(giConfiguration);
    }
    _configuration.globalIllumination->initialize(*engine.device(),engine.backendType(),count);
    if (!_configuration.reflections)
    {
        pipeline::RTReflectionConfiguration configuration;
        configuration.shaderRoot = _shaderRoot;
        _configuration.reflections = std::make_shared<pipeline::RTReflections>(configuration);
    }
    _configuration.reflections->initialize(*engine.device(),engine.backendType(),count);
    if (!_configuration.temporal)
    {
        pipeline::TemporalConfiguration configuration;
        configuration.shaderRoot = _shaderRoot;
        configuration.metadataShaderInterface = "draw.temporal.v2";
        configuration.reprojectionShaderInterface = "draw.temporal.v3";
        configuration.adaptiveShaderInterface = "draw.temporal.v4";
        configuration.momentShaderInterface = "draw.temporal.v5";
        _configuration.temporal = std::make_shared<pipeline::TemporalAccumulator>(configuration);
    }
    _configuration.temporal->initialize(*engine.device(),engine.backendType(),count);
    if (!_configuration.denoise)
    {
        pipeline::DenoiseConfiguration configuration;
        configuration.shaderRoot = _shaderRoot;
        _configuration.denoise = std::make_shared<pipeline::DenoiseFilter>(configuration);
    }
    _configuration.denoise->initialize(*engine.device(),engine.backendType(),count);
    if (_configuration.finalProcessor)
    {
        try
        {
            _configuration.finalProcessor->initialize(engine,_configuration.outputFormat,count);
        }
        catch (const std::runtime_error& error)
        {
            warnInitialization(std::string("Final processor initialization failed; original final color retained: ") + error.what());
            _configuration.finalProcessor->cleanup();
            _configuration.finalProcessor.reset();
        }
    }
    const auto ao = _configuration.ambientOcclusion;
    const auto gi = _configuration.globalIllumination;
    const auto reflection = _configuration.reflections;
    const auto reportUnavailable = [this](const auto& effect)
    {
        if (effect && !effect->available())
        {
            warnInitialization(effect->status());
        }
    };
    reportUnavailable(_configuration.softShadows);
    reportUnavailable(ao);
    reportUnavailable(gi);
    reportUnavailable(reflection);
    reportUnavailable(_configuration.temporal);
    reportUnavailable(_configuration.denoise);
    reportUnavailable(_configuration.finalProcessor);
    if ((ao->available() && !ao->noise()) || (gi->available() && !gi->noise()) ||
        (reflection->available() && !reflection->noise()))
    {
        try
        {
            _sharedNoise = std::make_shared<utils::BlueNoise>(*engine.device());
            _sharedNoise->build();
            if (!ao->noise())
            {
                ao->setNoise(_sharedNoise);
            }
            if (!gi->noise())
            {
                gi->setNoise(_sharedNoise);
            }
            if (!reflection->noise())
            {
                reflection->setNoise(_sharedNoise);
            }
            _noiseStatus = "Shared blue noise ready; individual controls select atlas or hash";
        }
        catch (const std::exception& error)
        {
            _noiseStatus = std::string("Shared blue noise unavailable: ")+error.what();
            warnInitialization(_noiseStatus + "; hash sampling active");
            _sharedNoise.reset();
        }
    }
    setIndirectLightingMode(_configuration.indirectLighting);
    // An explicitly injected unshadowed lighting contract opts this graph out
    // without forcing an incompatible replacement on query-capable devices.
    const bool unshadowedReplacement = std::any_of(_configuration.layers.begin(), _configuration.layers.end(), [](const auto& selection)
    {
        const auto& id = selection.configuration.contract.shaderInterface;
        return id.find("draw.raster.deferred") == 0 && id.find(".shadows.") == std::string::npos;
    });
    _queryShadows = _configuration.softShadows->available() && !unshadowedReplacement;
    const bool queryShadows = _queryShadows;
    const bool legacyLighting = std::any_of(_configuration.layers.begin(),_configuration.layers.end(),[](const auto& selection)
    {
        const auto& id = selection.configuration.contract.shaderInterface;
        return id.find("draw.raster.deferred") == 0 && id.find(".ao.") == std::string::npos;
    });
    _ambientOcclusion = _configuration.ambientOcclusion->available() && !legacyLighting;
    const bool oldLighting = std::any_of(_configuration.layers.begin(),_configuration.layers.end(),[](const auto& selection)
    {
        const auto& id = selection.configuration.contract.shaderInterface;
        return id.find("draw.raster.deferred") == 0 && id.find(".gi.") == std::string::npos;
    });
    _globalIllumination = _configuration.globalIllumination->available() && !oldLighting;
    const bool oldReflectionLighting = std::any_of(_configuration.layers.begin(),_configuration.layers.end(),[](const auto& selection)
    {
        const auto& id = selection.configuration.contract.shaderInterface;
        return id.find("draw.raster.deferred") == 0 && id.find(".reflections.") == std::string::npos;
    });
    _reflections = _configuration.reflections->available() && !oldReflectionLighting;
    auto supplied = std::move(_configuration.layers);
    _configuration.layers.clear();
    auto sky = std::make_shared<environment::SkyboxRenderer>();
    sky->settings().shaderRoot = _shaderRoot;
    std::vector<std::shared_ptr<pipeline::RenderLayer>> defaults{
        std::make_shared<pipeline::GBuffer>(_shaderRoot),
        std::make_shared<pipeline::DeferredLayer>(_shaderRoot, count, depth, queryShadows, _ambientOcclusion, _globalIllumination, _reflections),
        std::make_shared<pipeline::FinalLayer>(_shaderRoot, count), sky};
    const auto hdr = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    std::vector<pipeline::PassContract> contracts{pipeline::gbufferContract(depth), pipeline::deferredContract(depth, queryShadows, _ambientOcclusion, _globalIllumination, _reflections),
        pipeline::finalContract(_configuration.outputFormat, depth, true),
        {"draw.environment.skybox.v1", {}, {{"color", hdr, pipeline::ExtentDomain::Internal, gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled},
            {"depth", depth, pipeline::ExtentDomain::Internal, gpu::ImageUsage::DepthStencil}}}};
    for (size_t i = 0; i < defaults.size(); ++i)
    {
        for (auto it = supplied.begin(); it != supplied.end();)
        {
            if (it->configuration.contract.shaderInterface == contracts[i].shaderInterface)
            {
                defaults[i] = it->layer;
                it = supplied.erase(it);
                for (const auto& duplicate : supplied)
                {
                    if (duplicate.configuration.contract.shaderInterface == contracts[i].shaderInterface)
                    {
                        throw std::invalid_argument("Duplicate renderer stage replacement");
                    }
                }
            }
            else
            {
                ++it;
            }
        }
        pipeline::LayerConfiguration selected{_configuration.outputExtent, _configuration.internalExtent, contracts[i]};
        _configuration.layers.push_back({defaults[i], selected});
        // Store before initialization so cleanup covers partially initialized passes.
        defaults[i]->initialize(engine, selected);
    }
    if (!supplied.empty() || !std::dynamic_pointer_cast<environment::SkyboxPass>(_configuration.layers[3].layer))
    {
        throw std::invalid_argument("Unknown renderer stage or incompatible skybox replacement");
    }
    Renderer::validateConfiguration(_configuration);
    if (giUpsamplingCompatible())
    {
        try
        {
            initializeGIUpsampling();
        }
        catch (const std::exception& error)
        {
            cleanupGIUpsampling();
            warnInitialization(std::string("GI upsampling unavailable; original scaled GI sampling retained: ") + error.what());
        }
    }
    _drawLayout = engine.device()->createPipelineLayout(drawBindingLayout());
    _lightingLayout = engine.device()->createPipelineLayout(pipeline::deferredBindingLayout(queryShadows, _ambientOcclusion, _globalIllumination, _reflections));
    _frame.initialize(engine, _lightingLayout, count);
    _object.initialize(engine, _drawLayout, count);
    _lights.initialize(engine, _lightingLayout, count);
    _environmentBinding.initialize(engine, _lightingLayout, count, 8);
    _temporalSurface = std::make_unique<pipeline::TemporalSurfacePass>(_shaderRoot);
    _temporalSurface->initialize(engine,*_configuration.layers[0].layer,count);
    updateTemporalSurfaceStatus();
    if (_temporalSurfaceEnabled && !_temporalSurface->available())
    {
        warnInitialization(_temporalSurface->status());
    }
    for (auto it = _configuration.overlays.begin(); it != _configuration.overlays.end();)
    {
        ++_initializedOverlays; // Cleanup also covers partially initialized overlays.
        try
        {
            (*it)->initialize(engine, overlayConfiguration());
            ++it;
        }
        catch (const std::runtime_error& error)
        {
            warnInitialization(std::string("Optional overlay initialization failed; overlay omitted: ") + error.what());
            (*it)->cleanup();
            --_initializedOverlays;
            it = _configuration.overlays.erase(it);
        }
    }
    buildTargets();
    if (_configuration.environment)
    {
        _configuration.environment->validate(engine.device());
        _environment = _configuration.environment;
    }
    else
    {
        _generator = std::make_unique<environment::EnvironmentGenerator>(engine, _shaderRoot);
        try
        {
            _generator->synchronize(nullptr);
        }
        catch (const std::runtime_error& error)
        {
            warnInitialization(std::string("Initial environment generation failed; black environment active: ") + error.what());
            _generator->cleanup(); // Release partially generated maps before the minimal allocation.
            _generator->buildFallback();
        }
        _environment = _generator->resources();
    }
    std::dynamic_pointer_cast<environment::SkyboxPass>(_configuration.layers[3].layer)->setEnvironment(_environment);
}

void RendererDeferred::warnInitialization(const std::string& message)
{
    if (!_initializationWarnings.empty())
    {
        _initializationWarnings += '\n';
    }
    _initializationWarnings += message;
    std::cerr << "WARN: " << message << std::endl;
}

const std::string& RendererDeferred::initializationWarnings() const
{
    return _initializationWarnings;
}

bool RendererDeferred::temporalSurfaceEnabled() const
{
    return _temporalSurfaceEnabled;
}

void RendererDeferred::setTemporalSurfaceEnabled(bool value)
{
    _temporalSurfaceEnabled = value;
    updateTemporalSurfaceStatus();
}

bool RendererDeferred::temporalSurfaceAvailable() const
{
    return _temporalSurface && _temporalSurface->available();
}

const std::string& RendererDeferred::temporalSurfaceStatus() const
{
    return _temporalSurfaceStatus;
}

pipeline::TemporalDiagnosticView RendererDeferred::temporalDiagnosticView() const
{
    return _temporalDiagnosticView;
}

void RendererDeferred::setTemporalDiagnosticView(pipeline::TemporalDiagnosticView value)
{
    if (uint32_t(value) > uint32_t(pipeline::TemporalDiagnosticView::EffectiveCount))
    {
        throw std::invalid_argument("Unknown temporal diagnostic view");
    }
    _temporalDiagnosticView = value;
}

pipeline::TemporalSignal RendererDeferred::temporalDiagnosticSignal() const
{
    return _temporalDiagnosticSignal;
}

void RendererDeferred::setTemporalDiagnosticSignal(pipeline::TemporalSignal value)
{
    if (uint32_t(value) > uint32_t(pipeline::TemporalSignal::ReflectedRadiance))
    {
        throw std::invalid_argument("Unknown temporal diagnostic signal");
    }
    _temporalDiagnosticSignal = value;
}

uint64_t RendererDeferred::temporalDiagnosticLayer() const
{
    return _temporalDiagnosticLayer;
}

void RendererDeferred::setTemporalDiagnosticLayer(uint64_t value)
{
    _temporalDiagnosticLayer = value ? TransparentTemporalLayer : 0;
}

bool RendererDeferred::temporalDiagnosticsAvailable() const
{
    return temporalSurfaceAvailable() && _configuration.temporal && _configuration.temporal->diagnosticsAvailable();
}

bool RendererDeferred::temporalAdaptiveDiagnosticsAvailable() const
{
    return temporalSurfaceAvailable() && _configuration.temporal && _configuration.temporal->adaptiveDiagnosticsAvailable();
}

bool RendererDeferred::temporalSurfaceDiagnosticsAvailable() const
{
    return temporalSurfaceAvailable() && _configuration.temporal && _configuration.temporal->surfaceDiagnosticsAvailable();
}

pipeline::TemporalSurfaceMetadata RendererDeferred::temporalSurfaceMetadata(uint32_t slotIndex, uint64_t layer) const
{
    const auto& slot = _targets.at(slotIndex);
    if (layer == 0)
    {
        return {slot.opaqueTemporalSurface.token,slot.opaqueTemporalSurface.depth,
            slot.opaqueTemporalSurface.previousPosition,slot.opaqueTemporalSurface.previousNormal,slot.opaqueTemporalSurface.legacyCompatible,slot.opaqueTemporalSurface.motion};
    }
    const auto& metadata = slot.transparentTemporalSurface;
    return {metadata.token,metadata.depth,metadata.previousPosition,metadata.previousNormal,
        metadata.legacyCompatible,metadata.motion};
}

void RendererDeferred::updateTemporalSurfaceStatus()
{
    if (!_temporalSurface)
    {
        _temporalSurfaceStatus = "Not initialized";
    }
    else if (!_temporalSurface->available())
    {
        _temporalSurfaceStatus = _temporalSurface->status();
    }
    else if (!_temporalSurfaceEnabled)
    {
        _temporalSurfaceStatus = "Disabled: experimental receiver metadata requires explicit opt-in; native validation pending";
    }
    else
    {
        if (_configuration.temporal->surfaceReprojectionAvailable())
        {
            _temporalSurfaceStatus = "Enabled: experimental rigid receiver reprojection; native validation pending";
        }
        else
        {
            _temporalSurfaceStatus = _configuration.temporal->surfaceMetadataAvailable() ?
                "Enabled: static receiver validation; "+_configuration.temporal->surfaceReprojectionStatus() :
                _configuration.temporal->surfaceMetadataStatus();
        }
    }
}

pipeline::OverlayConfiguration RendererDeferred::overlayConfiguration() const
{
    return {_configuration.outputExtent, _configuration.outputFormat,
        _configuration.depthFormat, uint32_t(_targets.size())};
}

void RendererDeferred::buildTargets()
{
    using pipeline::RenderTarget;
    const auto size = _configuration.internalExtent;
    for (auto& slot : _targets)
    {
        Targets next;
        for (const auto& output : pipeline::gbufferContract(_configuration.depthFormat).outputs)
        {
            next.gbuffer.push_back(RenderTarget::create(*_engine, size, output.format, output.semantic));
        }
        next.opaqueDepth = RenderTarget::create(*_engine, size, _configuration.depthFormat, "preserved opaque depth");
        next.colorA = RenderTarget::create(*_engine, size, gpu::PixelFormat::R16G16B16A16_SFLOAT, "deferred HDR A");
        next.colorB = RenderTarget::create(*_engine, size, gpu::PixelFormat::R16G16B16A16_SFLOAT, "deferred HDR B");
        gpu::ImageDescription output;
        output.size = _configuration.outputExtent;
        output.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        output.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment |
            gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
        // The SDK writes storage HDR; the portable fallback only needs an attachment.
        if (_configuration.upscaler->available() && _configuration.upscaler->requirements().storageOutput)
        {
            output.usage = output.usage | gpu::ImageUsage::Storage;
        }
        output.debugName = "draw reconstructed linear HDR";
        next.reconstructed = _engine->device()->createImage(output);
        if (_queryShadows)
        {
            for (uint32_t i = 0; i < 2; ++i)
            {
                gpu::ImageDescription description;
                description.size = size;
                description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
                description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::ColorAttachment;
                description.debugName = "draw per-light query visibility";
                next.shadows.push_back(_engine->device()->createImage(description));
            }
        }
        if (_ambientOcclusion)
        {
            gpu::ImageDescription description;
            description.size = _configuration.ambientOcclusion->outputExtent(size);
            description.format = gpu::PixelFormat::R16_SFLOAT;
            description.usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
            description.debugName = "draw scalar ambient visibility";
            next.ao = _engine->device()->createImage(description);
        }
        if (_globalIllumination)
        {
            gpu::ImageDescription description;
            description.size = _configuration.globalIllumination->outputExtent(size);
            description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
            description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::ColorAttachment;
            next.gi = _engine->device()->createImage(description);
            if (_giUpsampling && giUpsamplingCompatible() && description.size != size)
            {
                description.size = size;
                description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage;
                description.debugName = "draw internal-resolution joint bilateral diffuse GI";
                next.giUpsampled = _engine->device()->createImage(description);
            }
        }
        if (_reflections)
        {
            gpu::ImageDescription description;
            description.size = _configuration.reflections->outputExtent(_configuration.outputExtent);
            description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
            description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::ColorAttachment;
            description.debugName = "draw output-relative reflected radiance/confidence";
            next.reflections = _engine->device()->createImage(description);
        }
        slot = std::move(next);
    }
}

void RendererDeferred::attachScene(std::shared_ptr<scene::Node> sceneRoot)
{
    if (!_engine)
    {
        throw std::logic_error("Initialize renderer before scene attachment");
    }
    {
        base::CpuProfileScope profile("renderer.waitIdle.attachScene");
        base::CpuProfileScope::count("renderer.attachSceneDrains", 1, "drains");
        _engine->device()->waitIdle();
    }
    resetGIUpsamplingSlots();
    if (_tracingResources)
    {
        _tracingResources->clear();
    }
    _scene = std::move(sceneRoot);
    _queue.clear();
    resetUpscaler();
    if (!_explicitCamera)
    {
        _view = _projection = glm::mat4{1};
    }
    for (auto& slot : _targets)
    {
        slot.tracing.reset();
        slot.queue.clear();
        slot.prepared = false;
    }
    if (_scene)
    {
        scene::ResizeViewportVisitor resize;
        resize.resizeViewport(_scene.get(), _configuration.internalExtent);
    }
    for (auto& selection : _configuration.layers)
    {
        selection.layer->attachScene(_scene);
    }
    _configuration.softShadows->attachScene(_scene);
    _configuration.ambientOcclusion->reset();
    _configuration.globalIllumination->reset();
    _configuration.reflections->reset();
    invalidateHistories();
    for (const auto& overlay : _configuration.overlays)
    {
        overlay->attachScene(_scene);
    }
    prepareFrame();
}

void RendererDeferred::resize(gpu::Size2D output, gpu::Size2D internal)
{
    if (!_engine || output.isZero() || internal.isZero())
    {
        throw std::invalid_argument("Renderer resize requires initialized nonzero extents");
    }
        {
        base::CpuProfileScope profile("renderer.waitIdle.resize");
        base::CpuProfileScope::count("renderer.resizeDrains", 1, "drains");
        _engine->device()->waitIdle();
    }
    resetGIUpsamplingSlots();
    _configuration.outputExtent = output;
    _configuration.internalExtent = internal;
    _configuration.upscaler->resize(upscaleConfiguration());
    _fallback->resize(upscaleConfiguration());
    for (auto& selection : _configuration.layers)
    {
        selection.configuration.outputExtent = output;
        selection.configuration.internalExtent = internal;
        selection.layer->resize(selection.configuration);
    }
    _shadowConfiguration.outputExtent = output;
    _shadowConfiguration.internalExtent = internal;
    _configuration.softShadows->resize(_shadowConfiguration);
    _configuration.ambientOcclusion->reset();
    _configuration.globalIllumination->reset();
    _configuration.reflections->reset();
    invalidateHistories();
    for (const auto& overlay : _configuration.overlays)
    {
        overlay->resize(overlayConfiguration());
    }
    buildTargets();
    if (_scene)
    {
        scene::ResizeViewportVisitor resize;
        resize.resizeViewport(_scene.get(), internal);
    }
}

bool RendererDeferred::prepareFrame()
{
    base::CpuProfileScope prepareProfile("renderer.prepareFrame");
    if (!_engine)
    {
        return false;
    }
    bool overlayChanged = false;
    base::CpuProfileScope overlayProfile("renderer.prepareOverlayFrames");
    for (const auto& overlay : _configuration.overlays)
    {
        overlayChanged = overlay->prepareFrame() || overlayChanged;
    }
    base::CpuProfileScope::count("renderer.preparedOverlays", _configuration.overlays.size());
    if (!_generator)
    {
        return overlayChanged;
    }
    if (!_scene)
    {
        return overlayChanged;
    }
    overlayProfile.next("renderer.prepareEnvironment");
    SceneInputs inputs(_scene.get());
    _scene->accept(&inputs);
    try
    {
        const bool generated = _generator->synchronize(inputs.environment);
        auto next = _generator->resources();
        if (!generated && next == _environment)
        {
            _environmentError.clear();
            return overlayChanged;
        }
        std::dynamic_pointer_cast<environment::SkyboxPass>(_configuration.layers[3].layer)->setEnvironment(next);
        _environment = std::move(next);
        invalidateHistories();
        _environmentError.clear();
        return true;
    }
    catch (const std::exception& error)
    {
        // Published previous resources remain usable; report failed replacement.
        _environmentError = error.what();
        return overlayChanged;
    }
}

void RendererDeferred::checkFrame(const FrameContext& frame) const
{
    if (&frame.engine != _engine || frame.frameSlot >= _targets.size() || frame.extent != _configuration.outputExtent ||
        !frame.colorTarget.isValid() || frame.colorTarget.device() != _engine->device() || frame.colorTarget.size() != frame.extent ||
        frame.colorTarget.pixelFormat() != _configuration.outputFormat ||
        !gpu::hasFlag(frame.colorTarget.usage(), gpu::ImageUsage::ColorAttachment) ||
        !gpu::hasFlag(frame.colorTarget.usage(), gpu::ImageUsage::Sampled) || frame.commandBuffer.hasActiveScope())
    {
        throw std::invalid_argument("Renderer frame engine/slot/extent/format or scope mismatch");
    }
}

void RendererDeferred::update(const FrameContext& frame)
{
    base::CpuProfileScope profile("renderer.update");
    base::CpuProfileScope stageProfile("renderer.extractQueueAndCamera");
    checkFrame(frame);
    auto& slot = _targets[frame.frameSlot];
    if (slot.prepared && slot.serial == frame.frameNumber)
    {
        throw std::logic_error("Renderer updates once per acquired presentation");
    }
    retireGIUpsamplingSlot(frame.frameSlot, frame.frameNumber);
    if (_scene)
    {
        scene::UpdateVisitor update;
        update.update(_scene.get(), frame.sceneDeltaSeconds);
        SceneInputs inputs(_scene.get());
        _scene->accept(&inputs);
        if (!_explicitCamera)
        {
            _temporalCameraIdentity = inputs.camera;
            _temporalCameraOwner = inputs.camera ? inputs.camera->weak_from_this().lock() : nullptr;
            _view = inputs.camera ? inputs.camera->viewMatrix() : glm::mat4{1};
            _projection = inputs.camera ? inputs.camera->projectionMatrix() : glm::mat4{1};
        }
        RenderQueueVisitor visitor;
        visitor.enqueue(_scene.get(), *_engine, slot.queue);
    }
    else
    {
        slot.queue.beginFrame(*_engine);
        _temporalCameraIdentity = nullptr;
        _temporalCameraOwner.reset();
    }
    slot.queue.sort(glm::vec3(glm::inverse(_view)[3]));
    _queue = slot.queue;
    stageProfile.next("renderer.temporalSignature");
    const auto signature = prepareTemporalSignature(slot.queue);
    const bool componentChanged = !_historyState || !signature->historyCompatible(*_historyState);
    const bool sceneMotion = !componentChanged && signature->hasTransformChanges(*_historyState);
    bool changed = componentChanged;
    std::vector<uint8_t> reference, policyReference;
    const bool referenceEnabled = !_signatureReuseEnabled || _signatureValidationEnabled;
    if (referenceEnabled)
    {
        reference = fullTemporalSignature(slot.queue);
        policyReference = historyPolicySignature(slot.queue);
        const std::vector<uint8_t> empty;
        const auto& previous = _historySignatureCurrent ? _historyPolicySignature :
            (_historyState ? _historyState->historyPolicyBytes() : empty);
        const bool referenceChanged = !_historyState || policyReference != previous;
        const auto& previousFull = _historySignatureCurrent ? _historySignature :
            (_historyState ? _historyState->flattened() : empty);
        const bool fullReferenceChanged = !_historyState || reference != previousFull;
        const bool fullComponentChanged = !_historyState || !signature->equals(*_historyState);
        if (_signatureValidationEnabled &&
            (signature->flattened() != reference || signature->historyPolicyBytes() != policyReference ||
             componentChanged != referenceChanged || fullComponentChanged != fullReferenceChanged))
        {
            throw std::logic_error("Cached temporal identity or history policy differs from reference serialization");
        }
        changed = _signatureReuseEnabled ? componentChanged : referenceChanged;
        base::CpuProfileScope::count("temporal.signatureReferenceBytes", reference.size(), "bytes");
    }
    if (changed)
    {
        signature->countResetCauses(_historyState.get());
        base::CpuProfileScope::count("temporal.signatureResets", 1);
        resetHistories();
    }
    if (!_temporalCorrespondence)
    {
        _temporalCorrespondence = std::make_shared<TemporalCorrespondence>();
    }
    slot.preparedCorrespondence = _temporalCorrespondence->prepare(*signature,frame.sceneFrameNumber,_historyGeneration);
    slot.recordedCorrespondence.reset();
    slot.opaqueTemporalSurface = {};
    slot.diagnostic.reset();
    slot.diagnosticView = _temporalDiagnosticView;
    slot.diagnosticSignal = _temporalDiagnosticSignal;
    slot.diagnosticLayer = _temporalDiagnosticLayer;
    slot.transparentTemporalSurface = {};
    updateTemporalSurfaceStatus();
    slot.recordTemporalSurfaces = _temporalSurfaceEnabled && temporalSurfaceAvailable() &&
        slot.preparedCorrespondence->identitiesValid;
    if (_temporalSurfaceEnabled && temporalSurfaceAvailable() && !slot.preparedCorrespondence->identitiesValid)
    {
        _temporalSurfaceStatus = "Fallback for current scene: nonunique or empty temporal surface identities";
    }
    slot.sceneMotion = !changed && sceneMotion;
    if (slot.sceneMotion)
    {
        base::CpuProfileScope::count("temporal.objectMotionObservations", 1);
    }
    // Accepted history state is independent of derived-cache/fingerprint lookups.
    _historyState = signature;
    _historySignatureCurrent = referenceEnabled;
    if (referenceEnabled)
    {
        _historySignature = std::move(reference);
        _historyPolicySignature = std::move(policyReference);
    }
    else
    {
        std::vector<uint8_t> empty;
        _historySignature.swap(empty); // Drop unused full-baseline storage.
        std::vector<uint8_t> emptyPolicy;
        _historyPolicySignature.swap(emptyPolicy);
    }
    stageProfile.next("renderer.beginTemporalAndFrameBindings");
    _configuration.temporal->beginFrame(frame.frameSlot,frame.frameNumber,frame.sceneFrameNumber,slot.sceneMotion);
    // Retire metadata invocations even when replay was disabled since the last
    // use of this slot. Only scene updates reach this point.
    if (_temporalSurface)
    {
        _temporalSurface->beginFrame(frame.frameSlot,frame.frameNumber);
    }
    _configuration.denoise->beginFrame(frame.frameSlot,frame.frameNumber);
    slot.camera = _upscaleCamera;
    slot.camera.view = _view;
    slot.camera.projection = _projection;
    // Perspective projection coefficients provide actual near/far/FOV for scene
    // cameras and explicit matrices. Orthographic/identity uses unjittered fallback.
    if (std::abs(_projection[2][3] + 1.0f) < 0.0001f && std::abs(_projection[3][3]) < 0.0001f)
    {
        const float a = _projection[2][2], b = _projection[3][2];
        if (a != 0 && a != -1)
        {
            slot.camera.nearPlane = b / a;
            slot.camera.farPlane = b / (a + 1);
            slot.camera.verticalFov = 2 * std::atan(1.0f / std::abs(_projection[1][1]));
        }
    }
    slot.jitteredProjection = _configuration.upscaler->prepare(slot.camera, frame.sceneFrameNumber);
    _frame.update(frame, _view, slot.jitteredProjection);
    stageProfile.next("renderer.objectBindings");
    _object.prepare(frame, slot.queue);
    stageProfile.next("renderer.lightAndEnvironmentBindings");
    _lights.update(frame, slot.queue.lights());
    _environmentBinding.update(frame, _environment);
    // One immutable scene snapshot per acquired frame, shared by every RT pass.
    // Reuse immutable tables and completed acceleration when their keys match.
    const bool shadowTracing = _queryShadows && _configuration.softShadows->available() &&
        _configuration.softShadows->settings().enabled;
    const bool aoTracing = _ambientOcclusion && _configuration.ambientOcclusion->enabled() &&
        _configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion;
    const bool giTracing = _globalIllumination && _configuration.globalIllumination->enabled() &&
        _configuration.indirectLighting == IndirectLightingMode::GlobalIllumination;
    const bool reflectionTracing = _reflections && _configuration.reflections->enabled();
    base::CpuProfileScope::count("tracing.sceneSnapshotsWithConsumer", shadowTracing || aoTracing || giTracing || reflectionTracing ? 1 : 0);
    base::CpuProfileScope::count("tracing.shadowConsumer", shadowTracing ? 1 : 0);
    base::CpuProfileScope::count("tracing.aoConsumer", aoTracing ? 1 : 0);
    base::CpuProfileScope::count("tracing.giConsumer", giTracing ? 1 : 0);
    base::CpuProfileScope::count("tracing.reflectionConsumer", reflectionTracing ? 1 : 0);
    stageProfile.next("renderer.retireTracingSnapshot");
    slot.tracing.reset();
    stageProfile.next("renderer.prepareTracingSnapshot");
    if (shadowTracing || aoTracing || giTracing || reflectionTracing)
    {
        if (!_tracingResources)
        {
            // Use a stable common profile, including currently disabled effects,
            // so toggling an effect never changes the shared table's addressing.
            auto capacity = gpu::MaxSampledResourceTableCapacity;
            if (_ambientOcclusion)
            {
                capacity = std::min(capacity, _configuration.ambientOcclusion->tableCapacity());
            }
            if (_globalIllumination)
            {
                capacity = std::min(capacity, _configuration.globalIllumination->tableCapacity());
            }
            if (_reflections)
            {
                capacity = std::min(capacity, _configuration.reflections->tableCapacity());
            }
            if (_queryShadows)
            {
                capacity = std::min(capacity, _configuration.softShadows->alphaTextureCapacity());
            }
            _tracingResources = std::make_shared<pipeline::TracingSceneResources>(*_engine->device(), capacity);
        }
        slot.tracing = _tracingResources->prepare(slot.queue,
            shadowTracing ? _configuration.softShadows->alphaTextureCapacity() : 0);
    }
    stageProfile.next("renderer.prepareEffects");
    if (_queryShadows)
    {
        _configuration.softShadows->prepare(frame, slot.queue, _view, slot.jitteredProjection, slot.tracing);
    }
    if (_ambientOcclusion)
    {
        if (_configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion)
        {
            _configuration.ambientOcclusion->prepareSnapshot(frame.frameSlot,frame.frameNumber,slot.tracing);
        }
        else
        {
            _configuration.ambientOcclusion->prepareSnapshot(frame.frameSlot,frame.frameNumber,nullptr);
        }
    }
    if (_globalIllumination)
    {
        if (_configuration.indirectLighting == IndirectLightingMode::GlobalIllumination)
        {
            _configuration.globalIllumination->prepareSnapshot(frame.frameSlot,frame.frameNumber,slot.tracing);
        }
        else
        {
            _configuration.globalIllumination->prepareSnapshot(frame.frameSlot,frame.frameNumber,nullptr);
        }
    }
    if (_reflections)
    {
        _configuration.reflections->prepareSnapshot(frame.frameSlot,frame.frameNumber,
            reflectionTracing ? slot.tracing : nullptr);
    }
    std::dynamic_pointer_cast<environment::SkyboxPass>(_configuration.layers[3].layer)->setCamera(_view, slot.jitteredProjection);
    for (auto& selection : _configuration.layers)
    {
        selection.layer->update(frame);
    }
    const pipeline::OverlayContext overlayContext{frame, _scene.get(), slot.queue, slot.camera.view, slot.camera.projection};
    for (const auto& overlay : _configuration.overlays)
    {
        overlay->update(overlayContext);
    }
    slot.serial = frame.frameNumber;
    slot.prepared = true;
    slot.linearRecorded = slot.finalRecorded = false;
    slot.finalItem = nullptr;
}

void RendererDeferred::dispatch(size_t stage, const FrameContext& frame,
    const std::vector<pipeline::ImageBinding>& inputs, const std::vector<pipeline::ImageBinding>& outputs,
    const pipeline::RasterFrame* raster)
{
    base::CpuProfileScope profile(stage == 0 ? "layer.gbuffer" :
        stage == 1 ? "layer.deferredLighting" : stage == 2 ? "layer.final" :
        stage == 3 ? "layer.skybox" : "layer.other");
    gpu::GpuTimingScope gpuProfile(frame.commandBuffer, stage == 0 ? "layer.gbuffer" :
        stage == 1 ? "layer.deferredLighting" : stage == 2 ? "layer.final" :
        stage == 3 ? "layer.skybox" : "layer.other",
        raster && (raster->item || !raster->transparentItems.empty()) ? TransparentTemporalLayer : 0,
        stage != 2 && raster && (raster->item || !raster->transparentItems.empty()));
    const auto& selected = _configuration.layers.at(stage);
    Renderer::recordLayer(selected, {frame, selected.configuration, inputs, outputs, _scene.get(), raster});
}

bool RendererDeferred::supportsLinearRecording() const
{
    return true;
}

void RendererDeferred::record(const FrameContext& frame)
{
    auto linear = recordLinear(frame);
    recordFinal(frame, *linear);
}

std::shared_ptr<gpu::Image> RendererDeferred::recordLinear(const FrameContext& frame)
{
    base::CpuProfileScope profile("renderer.recordLinear");
    checkFrame(frame);
    auto& slot = _targets[frame.frameSlot];
    if (!slot.prepared || slot.serial != frame.frameNumber || slot.linearRecorded)
    {
        throw std::logic_error("Update the acquired renderer slot before recording");
    }
    // Resolve the predecessor at recording time, after any intervening ordered
    // submission. Fingerprint queries and update-only work never publish it.
    auto correspondence = _temporalCorrespondence->record(*slot.preparedCorrespondence,
        frame.sceneFrameNumber,_historyGeneration,frame.commandBuffer.recordingCompletion(),
        {{slot.camera.view,slot.camera.projection,slot.jitteredProjection,true},_configuration.internalExtent});
    slot.recordedCorrespondence = correspondence;
    if (slot.tracing)
    {
        slot.tracing->build(frame.commandBuffer);
    }
    pipeline::RasterFrame raster{&slot.queue, nullptr, &_frame, &_object, &_lights, &_environmentBinding};
    std::vector<pipeline::TemporalSurfaceInstance> surfaceInstances;
    if (slot.recordTemporalSurfaces)
    {
        surfaceInstances.reserve(slot.queue.items().size());
        for (const auto& item : slot.queue.items())
        {
            // Canonical correspondence is node/submesh. objectId is only the
            // current frame's selector for the existing raster object bindings.
            const auto found = correspondence->instances.find({item.nodeId,item.submeshIndex});
            if (found != correspondence->instances.end())
            {
                surfaceInstances.push_back({item.objectId,found->second.token,found->second.previousValid,
                    found->second.previousWorld,true});
            }
        }
    }
    const pipeline::TemporalSurfaceCamera surfaceCamera{
        correspondence->camera.transforms.projection*correspondence->camera.transforms.view,
        correspondence->previousCamera.transforms.projection*correspondence->previousCamera.transforms.view};
    std::vector<pipeline::ImageBinding> gbuffer;
    const auto& contract = _configuration.layers[0].configuration.contract;
    for (size_t i = 0; i < contract.outputs.size(); ++i)
    {
        gbuffer.push_back({contract.outputs[i].semantic, slot.gbuffer.at(i).get()});
    }
    dispatch(0, frame, {}, gbuffer, &raster);
    if (slot.recordTemporalSurfaces)
    {
        gpu::GpuTimingScope timing(frame.commandBuffer,"renderer.temporalSurface",0,false);
        slot.opaqueTemporalSurface = _temporalSurface->record(frame,raster,slot.gbuffer[0]->size(),surfaceInstances,nullptr,0,0,surfaceCamera);
    }
    pipeline::RenderTarget::copy(frame.commandBuffer, *slot.gbuffer[6], *slot.opaqueDepth);
    pipeline::RenderTarget::clear(frame.commandBuffer, *slot.colorA);
    dispatch(3, frame, {}, {{"color", slot.colorA.get()}, {"depth", slot.opaqueDepth.get()}});
    auto recordEffects = [&](uint64_t layer)
    {
        auto* surfaceDepth = layer ? slot.opaqueDepth.get() : slot.gbuffer[6].get();
        auto resolveSignal = [&](pipeline::TemporalSignal kind, gpu::Image& signal, gpu::Image* material = nullptr,
            float maxRoughness = 1.0f, std::shared_ptr<gpu::Image>* selectedOwner = nullptr)
        {
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = surfaceDepth;
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*slot.camera.view);
            const pipeline::TemporalView view{slot.camera.view,slot.camera.projection,slot.jitteredProjection,true};
            const auto accumulated = [&]
            {
                gpu::GpuTimingScope timing(frame.commandBuffer, "effect.temporal.record", layer, layer != 0);
                auto metadata = temporalSurfaceMetadata(frame.frameSlot,layer);
                // Injected legacy components retain the previous admission contract.
                if (!_configuration.temporal->surfaceMetadataAvailable() ||
                    (!_configuration.temporal->surfaceReprojectionAvailable() && !metadata.legacyCompatible))
                {
                    metadata = {};
                }
                return _configuration.temporal->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                    frame.sceneFrameNumber,_historyGeneration,{kind,0,layer},signal,surface,view,material,maxRoughness,metadata,
                    slot.diagnosticView == pipeline::TemporalDiagnosticView::LocalHistory &&
                        slot.diagnosticSignal == kind && slot.diagnosticLayer == layer,
                    uint32_t(slot.diagnosticView) >= uint32_t(pipeline::TemporalDiagnosticView::CurrentWeight) &&
                        slot.diagnosticSignal == kind && slot.diagnosticLayer == layer});
            }();
            pipeline::DenoiseSignal filteredKind = pipeline::DenoiseSignal::Visibility;
            if (kind == pipeline::TemporalSignal::DiffuseRadiance)
            {
                filteredKind = pipeline::DenoiseSignal::DiffuseRadiance;
            }
            else if (kind == pipeline::TemporalSignal::ReflectedRadiance)
            {
                filteredKind = pipeline::DenoiseSignal::ReflectedRadiance;
            }
            gpu::GpuTimingScope denoiseTiming(frame.commandBuffer, "effect.denoise.record", layer, layer != 0);
            const auto filtered = _configuration.denoise->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                {filteredKind,0,layer},*accumulated.image,surface,slot.camera.view,material,maxRoughness});
            if (selectedOwner)
            {
                *selectedOwner = filtered.owner;
                if (!*selectedOwner && filtered.image == accumulated.image)
                {
                    *selectedOwner = accumulated.owner;
                }
                if (!*selectedOwner && filtered.image == &signal)
                {
                    *selectedOwner = std::dynamic_pointer_cast<gpu::Image>(signal.weak_from_this().lock());
                }
            }
            return filtered.image;
        };
        std::vector<pipeline::ImageBinding> inputs;
        if (_queryShadows)
        {
            gpu::GpuTimingScope gpuProfile(frame.commandBuffer, "effect.shadows.record", layer, layer != 0);
            std::vector<pipeline::ImageBinding> visibility{{"shadow0", slot.shadows[0].get()}, {"shadow1", slot.shadows[1].get()}};
            const std::vector<pipeline::ImageBinding> sources{{"normalEmission", slot.gbuffer[1].get()}, {"depth", surfaceDepth}};
            pipeline::LayerSelection selection{_configuration.softShadows, _shadowConfiguration};
            Renderer::recordLayer(selection, {frame, selection.configuration, sources, visibility, _scene.get(), &raster});
            inputs.insert(inputs.end(), visibility.begin(), visibility.end());
        }
        if (_ambientOcclusion)
        {
            gpu::GpuTimingScope gpuProfile(frame.commandBuffer, "effect.ao.record", layer, layer != 0);
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = surfaceDepth;
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*slot.camera.view);
            _configuration.ambientOcclusion->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                uint32_t(frame.sceneFrameNumber),surface,*slot.ao});
            auto* signal = slot.ao.get();
            if (_configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion && _configuration.ambientOcclusion->enabled())
            {
                signal = resolveSignal(pipeline::TemporalSignal::Visibility,*signal);
            }
            inputs.push_back({"ambientOcclusion",signal});
        }
        if (_globalIllumination)
        {
            gpu::GpuTimingScope gpuProfile(frame.commandBuffer, "effect.gi.record", layer, layer != 0);
            if (_configuration.indirectLighting == IndirectLightingMode::GlobalIllumination)
            {
                pipeline::SurfaceInputs surface;
                surface.normal = slot.gbuffer[1].get();
                surface.depthOrPosition = surfaceDepth;
                surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*slot.camera.view);
                _configuration.globalIllumination->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                    uint32_t(frame.sceneFrameNumber),surface,*slot.gi,slot.queue.lights(),_environment});
            }
            else
            {
                pipeline::RenderTarget::clear(frame.commandBuffer,*slot.gi,{0,0,0,0});
            }
            auto* signal = slot.gi.get();
            if (_configuration.indirectLighting == IndirectLightingMode::GlobalIllumination && _configuration.globalIllumination->enabled())
            {
                std::shared_ptr<gpu::Image> selectedOwner;
                signal = resolveSignal(pipeline::TemporalSignal::DiffuseRadiance,*signal,nullptr,1.0f,&selectedOwner);
                if (signal == slot.gi.get())
                {
                    selectedOwner = slot.gi;
                }
                if (_giUpsampling && giUpsamplingCompatible() && slot.giUpsampled && signal &&
                    selectedOwner && selectedOwner.get() == signal && signal->size() == slot.gi->size() &&
                    signal->size() != _configuration.internalExtent)
                {
                    // Transparent GI deliberately keeps the producer's opaque-depth guide;
                    // temporal metadata depth does not describe that producer's receiver.
                    signal = recordGIUpsampling(frame.commandBuffer,selectedOwner,slot.gbuffer[1],
                        layer ? slot.opaqueDepth : slot.gbuffer[6],
                        glm::inverse(slot.jitteredProjection*slot.camera.view),slot.camera.view,
                        slot.giUpsampled,frame.frameSlot,frame.frameNumber,layer).get();
                }
            }
            inputs.push_back({"globalIllumination",signal});
        }
        if (_reflections)
        {
            gpu::GpuTimingScope gpuProfile(frame.commandBuffer, "effect.reflections.record", layer, layer != 0);
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = surfaceDepth;
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*slot.camera.view);
            const auto camera = glm::vec3(glm::inverse(slot.camera.view)[3]);
            _configuration.reflections->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                uint32_t(frame.sceneFrameNumber),surface,*slot.reflections,*slot.gbuffer[2],
                _configuration.outputExtent,camera,slot.queue.lights(),_environment});
            auto* signal = slot.reflections.get();
            if (_configuration.reflections->enabled())
            {
                signal = resolveSignal(pipeline::TemporalSignal::ReflectedRadiance,*signal,slot.gbuffer[2].get(),
                    _configuration.reflections->maxRoughness());
            }
            inputs.push_back({"reflections",signal});
        }
        return inputs;
    };
    auto composite = [&](gpu::Image* background, gpu::Image* output, uint64_t layer,
        const std::vector<pipeline::ImageBinding>& effects)
    {
        base::CpuProfileScope compositeCpu("renderer.composite");
        gpu::GpuTimingScope compositeProfile(frame.commandBuffer, "renderer.composite", layer, layer != 0);
        auto inputs = gbuffer;
        inputs[6].image = layer ? slot.opaqueDepth.get() : slot.gbuffer[6].get();
        inputs.push_back({"background",background});
        inputs.insert(inputs.end(),effects.begin(),effects.end());
        dispatch(1, frame, inputs, {{"color", output}}, &raster);
    };
    const auto opaqueEffects = recordEffects(0);
    composite(slot.colorA.get(),slot.colorB.get(),0,opaqueEffects);
    gpu::Image* current = slot.colorB.get();
    gpu::Image* next = slot.colorA.get();
    // Merge both transparent classes into one stable far-to-near order.
    std::vector<const RenderQueue::Item*> transparent;
    for (auto type : {RenderQueueType::Transparent, RenderQueueType::SolidTransparent})
    {
        for (const auto& item : slot.queue.queue(type))
        {
            transparent.push_back(&item);
        }
    }
    const auto cameraDepth = [&](const RenderQueue::Item* item)
    {
        glm::vec4 center(0,0,0,1);
        if (const auto bounds = item->drawMesh->uploadedBounds(item->submeshIndex))
        {
            center = glm::vec4((bounds->minimum+bounds->maximum)*0.5f,1);
        }
        const auto viewPosition = slot.camera.view*item->worldMatrix*center;
        const auto clipPosition = slot.camera.projection*viewPosition;
        // Match the depth test for perspective, orthographic and explicit
        // identity projections; radial distance gives the wrong overlap order.
        const float depth = std::abs(clipPosition.w) > 1e-8f ? clipPosition.z/clipPosition.w : -viewPosition.z;
        return std::isfinite(depth) ? depth : 0.0f;
    };
    std::stable_sort(transparent.begin(), transparent.end(), [&](const auto* a, const auto* b)
    {
        return cameraDepth(a) > cameraDepth(b);
    });
    // Custom shaders/passes may deform vertices or write outside coverage.
    const auto* gbufferLayer = _configuration.layers[0].layer.get();
    const auto* lightingLayer = _configuration.layers[1].layer.get();
    const auto* finalLayer = _configuration.layers[2].layer.get();
    const auto* lighting = dynamic_cast<const pipeline::DeferredLayer*>(lightingLayer);
    const auto* final = dynamic_cast<const pipeline::FinalLayer*>(finalLayer);
    const bool regions = _transparentRegionsEnabled &&
        typeid(*gbufferLayer) == typeid(pipeline::GBuffer) &&
        typeid(*lightingLayer) == typeid(pipeline::DeferredLayer) &&
        typeid(*finalLayer) == typeid(pipeline::FinalLayer) &&
        lighting && lighting->debugView() == pipeline::LightingDebugView::Final &&
        final && final->debugView() == pipeline::FinalDebugView::Final;
    const auto extent = slot.gbuffer[0]->size();
    auto transparentGbuffer = gbuffer;
    transparentGbuffer[6].image = slot.opaqueDepth.get();
    const uint64_t fullArea = uint64_t(extent.width)*extent.height;
    struct TransparentDraw {
        const RenderQueue::Item* item;
        std::optional<gpu::ScissorRegion> region;
    };
    std::vector<TransparentDraw> transparentDraws;
    std::vector<const RenderQueue::Item*> transparentReceivers;
    transparentDraws.reserve(transparent.size());
    transparentReceivers.reserve(transparent.size());
    {
        base::CpuProfileScope transparentCpu("renderer.transparent");
        gpu::GpuTimingScope transparentTiming(frame.commandBuffer, "renderer.transparent");
        if (!_inspectOpaqueLayer)
        {
            for (const auto* item : transparent)
            {
                std::optional<gpu::ScissorRegion> region;
                if (regions)
                {
                    const auto bounds = item->drawMesh->uploadedBounds(item->submeshIndex);
                    if (bounds && item->worldMatrix[0][3] == 0 && item->worldMatrix[1][3] == 0 &&
                        item->worldMatrix[2][3] == 0 && item->worldMatrix[3][3] == 1)
                    {
                        region = pipeline::projectProcessingRegion(bounds->minimum, bounds->maximum,
                            slot.jitteredProjection*slot.camera.view*item->worldMatrix, extent);
                    }
                }
                base::CpuProfileScope::count("transparent.fullPixels",fullArea,"pixels");
                if (region && (!region->width || !region->height))
                {
                    base::CpuProfileScope::count("transparent.skippedLayers",1);
                    continue; // Full-scene tracing snapshots still include offscreen geometry.
                }
                const auto area = region ? uint64_t(region->width)*region->height : fullArea;
                base::CpuProfileScope::count("transparent.rasterPixels",area,"pixels");
                base::CpuProfileScope::count("transparent.compositionPixels",area,"pixels");
                base::CpuProfileScope::count("transparent.fullExtentLayers",region ? 0 : 1);
                transparentDraws.push_back({item,region});
                transparentReceivers.push_back(item);
            }
        }
        if (!transparentReceivers.empty())
        {
            // Build the topmost transparent receiver once. Far-to-near color
            // overwrites select the last covered surface without writing depth.
            raster.item = nullptr;
            raster.region.reset();
            raster.transparentItems = {transparentReceivers.data(),transparentReceivers.size()};
            base::CpuProfileScope::count("transparent.receiverDomainPixels",fullArea,"pixels");
            base::CpuProfileScope::count("transparent.effectLayers",1);
            dispatch(0,frame,{},transparentGbuffer,&raster);
            if (slot.recordTemporalSurfaces)
            {
                gpu::GpuTimingScope timing(frame.commandBuffer,"renderer.temporalSurface",TransparentTemporalLayer,true);
                slot.transparentTemporalSurface = _temporalSurface->record(frame,raster,extent,surfaceInstances,
                    slot.opaqueDepth.get(),0,TransparentTemporalLayer,surfaceCamera);
            }
            const auto transparentEffects = recordEffects(TransparentTemporalLayer);
            raster.transparentItems = {};
            for (const auto& draw : transparentDraws)
            {
                raster.item = draw.item;
                raster.region = draw.region;
                // Preserve behind-surface color/refraction while sharing the
                // complete layer's already resolved effects across all items.
                if (raster.region)
                {
                    pipeline::RenderTarget::copy(frame.commandBuffer,*current,*next);
                    base::CpuProfileScope::count("transparent.backgroundCopyPixels",fullArea,"pixels");
                }
                dispatch(0,frame,{},transparentGbuffer,&raster);
                composite(current,next,TransparentTemporalLayer,transparentEffects);
                std::swap(current,next);
            }
        }
    }
    slot.finalItem = raster.item;
    if (slot.diagnosticView != pipeline::TemporalDiagnosticView::Off)
    {
        gpu::GpuTimingScope timing(frame.commandBuffer,"renderer.temporalDiagnostic",slot.diagnosticLayer,slot.diagnosticLayer != 0);
        slot.diagnostic = _configuration.temporal->recordDiagnostic(frame.commandBuffer,frame.frameSlot,frame.frameNumber,
            {slot.diagnosticSignal,0,slot.diagnosticLayer},temporalSurfaceMetadata(frame.frameSlot,slot.diagnosticLayer),slot.diagnosticView);
        if (slot.diagnostic)
        {
            current = slot.diagnostic.get();
        }
    }
    base::CpuProfileScope reconstructionCpu("renderer.reconstruction");
    gpu::GpuTimingScope reconstructionTiming(frame.commandBuffer, "renderer.reconstruction");
    UpscalerInput upscale{frame, *current, *slot.opaqueDepth, *slot.reconstructed, slot.camera, slot.jitteredProjection};
    // Always borrow the opaque result captured before the transparent loop.
    // Transparent motion is not motion of the composited/refraction color.
    upscale.opaqueMotion = {slot.opaqueTemporalSurface.motion.get(),slot.opaqueTemporalSurface.token.get(),
        slot.opaqueTemporalSurface.motion,slot.opaqueTemporalSurface.token};
    if (slot.diagnostic || !_configuration.upscaler->available() || !_configuration.upscaler->record(upscale))
    {
        _fallback->record(upscale);
    }
    frame.commandBuffer.transition(slot.reconstructed.get(), gpu::ImageLayout::ShaderReadOnly);
    slot.linearRecorded = true;
    _temporalCorrespondence->publish(std::move(correspondence));
    return slot.reconstructed;
}

void RendererDeferred::recordFinal(const FrameContext& frame, gpu::Image& linearColor)
{
    checkFrame(frame);
    auto& slot = _targets.at(frame.frameSlot);
    if (!slot.prepared || !slot.linearRecorded || slot.finalRecorded || slot.serial != frame.frameNumber || linearColor.device() != _engine->device() ||
        linearColor.size() != _configuration.outputExtent ||
        linearColor.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT)
    {
        throw std::invalid_argument("Final scene conversion requires a matching prepared slot and linear HDR image");
    }
    pipeline::RasterFrame raster{&slot.queue, slot.finalItem, &_frame, &_object, &_lights, &_environmentBinding};
    std::vector<pipeline::ImageBinding> gbuffer;
    const auto& contract = _configuration.layers[0].configuration.contract;
    for (size_t i = 0; i < contract.outputs.size(); ++i)
    {
        gbuffer.push_back({contract.outputs[i].semantic, slot.gbuffer.at(i).get()});
    }
    std::vector<pipeline::ImageBinding> finalInputs{{"color", &linearColor}};
    finalInputs.insert(finalInputs.end(), gbuffer.begin(), gbuffer.end());
    dispatch(2, frame, finalInputs, {{"output", &frame.colorTarget}}, &raster);
    if (_configuration.finalProcessor)
    {
        _configuration.finalProcessor->record(frame);
        if (frame.commandBuffer.hasActiveScope())
        {
            throw std::logic_error("Final processor left an active command scope");
        }
    }
    const pipeline::OverlayContext overlayContext{frame, _scene.get(), slot.queue, slot.camera.view, slot.camera.projection};
    for (const auto& overlay : _configuration.overlays)
    {
        if (frame.commandBuffer.hasActiveScope())
        {
            throw std::logic_error("Overlay must begin outside a command scope");
        }
        overlay->record(overlayContext);
        if (frame.commandBuffer.hasActiveScope())
        {
            throw std::logic_error("Overlay left an active command scope");
        }
    }
    frame.commandBuffer.transition(&frame.colorTarget, gpu::ImageLayout::ShaderReadOnly);
    slot.finalRecorded = true;
}

std::vector<uint8_t> RendererDeferred::sceneFingerprint() const
{
    const auto signature = _signatureReuseEnabled || _signatureValidationEnabled ? prepareTemporalSignature(_queue) : nullptr;
    auto result = _signatureReuseEnabled ? signature->flattened() : fullTemporalSignature(_queue);
    if (_signatureValidationEnabled &&
        (_signatureReuseEnabled ? result != fullTemporalSignature(_queue) : signature->flattened() != result))
    {
        throw std::logic_error("Cached scene fingerprint differs from full serialization");
    }
    const auto* view = reinterpret_cast<const uint8_t*>(&_view);
    result.insert(result.end(), view, view+sizeof(_view));
    const auto* generation = reinterpret_cast<const uint8_t*>(&_historyGeneration);
    result.insert(result.end(), generation, generation+sizeof(_historyGeneration));
    return result;
}

void RendererDeferred::setCamera(const glm::mat4& view, const glm::mat4& projection)
{
    if (!_explicitCamera)
    {
        resetUpscaler();
    }
    _view = view;
    _projection = projection;
    _explicitCamera = true;
}

void RendererDeferred::useSceneCamera()
{
    resetUpscaler();
    _explicitCamera = false;
    _view = _projection = glm::mat4{1};
}

pipeline::FinalSettings& RendererDeferred::finalSettings()
{
    auto pass = std::dynamic_pointer_cast<pipeline::FinalLayer>(_configuration.layers.at(2).layer);
    if (!pass)
    {
        throw std::logic_error("The injected final pass exposes its own settings API");
    }
    return pass->settings;
}

const RenderQueue& RendererDeferred::queue() const
{
    return _queue;
}

std::shared_ptr<pipeline::TracingSceneResources> RendererDeferred::tracingResources() const
{
    return _tracingResources;
}

std::shared_ptr<pipeline::SoftShadowPass> RendererDeferred::softShadows() const
{
    return _configuration.softShadows;
}

std::shared_ptr<pipeline::RTAmbientOcclusion> RendererDeferred::ambientOcclusion() const
{
    return _configuration.ambientOcclusion;
}

bool RendererDeferred::usesAmbientOcclusion() const
{
    return _ambientOcclusion;
}

bool RendererDeferred::ambientOcclusionEnabled() const
{
    return _ambientOcclusion && _configuration.ambientOcclusion->enabled();
}

bool RendererDeferred::inspectOpaqueLayer() const
{
    return _inspectOpaqueLayer;
}

void RendererDeferred::setInspectOpaqueLayer(bool value)
{
    if (_inspectOpaqueLayer != value)
    {
        _inspectOpaqueLayer = value;
        resetUpscaler();
    }
}

bool RendererDeferred::usesQueryShadows() const
{
    return _queryShadows;
}

environment::EnvironmentGenerator* RendererDeferred::environmentGenerator() const
{
    return _generator.get();
}

const std::string& RendererDeferred::environmentError() const
{
    return _environmentError;
}

UpscalerConfiguration RendererDeferred::upscaleConfiguration() const
{
    return {_configuration.internalExtent, _configuration.outputExtent, uint32_t(_targets.size()), _shaderRoot};
}

bool RendererDeferred::needsReconfiguration() const
{
    const bool reflectionResize = _reflections && !_targets.empty() && _targets.front().reflections &&
        _targets.front().reflections->size() != _configuration.reflections->outputExtent(_configuration.outputExtent);
    const bool giResize = _globalIllumination && !_targets.empty() && _targets.front().gi &&
        _targets.front().gi->size() != _configuration.globalIllumination->outputExtent(_configuration.internalExtent);
    const bool aoResize = _ambientOcclusion && !_targets.empty() && _targets.front().ao &&
        _targets.front().ao->size() != _configuration.ambientOcclusion->outputExtent(_configuration.internalExtent);
    return reflectionResize || giResize || aoResize || (_configuration.upscaler && _configuration.upscaler->needsRebuild());
}

std::shared_ptr<Upscaler> RendererDeferred::upscaler() const
{
    return _configuration.upscaler;
}

std::shared_ptr<ResolutionSettings> RendererDeferred::resolution() const
{
    return _configuration.resolution;
}

void RendererDeferred::resetUpscaler()
{
    invalidateHistories();
}

void RendererDeferred::invalidateHistories()
{
    base::CpuProfileScope::count("temporal.explicitOrLifecycleResets", 1);
    resetHistories();
}

void RendererDeferred::resetHistories()
{
    if (_historyGeneration == UINT64_MAX)
    {
        throw std::overflow_error("Renderer history generation exhausted");
    }
    ++_historyGeneration;
    if (_temporalCorrespondence)
    {
        _temporalCorrespondence->invalidate();
    }
    if (_configuration.temporal)
    {
        _configuration.temporal->invalidate();
    }
    if (_configuration.upscaler)
    {
        _configuration.upscaler->reset();
    }
}

uint64_t RendererDeferred::historyGeneration() const
{
    return _historyGeneration;
}

std::shared_ptr<pipeline::TemporalAccumulator> RendererDeferred::temporal() const
{
    return _configuration.temporal;
}

void RendererDeferred::setCameraParameters(float nearPlane, float farPlane, float fov)
{
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) || !std::isfinite(fov) ||
        nearPlane <= 0 || farPlane <= nearPlane || fov <= 0 || fov >= 3.14159265f)
    {
        throw std::invalid_argument("Invalid upscaler camera planes/FOV");
    }
    _upscaleCamera.nearPlane = nearPlane;
    _upscaleCamera.farPlane = farPlane;
    _upscaleCamera.verticalFov = fov;
    resetUpscaler();
}

void RendererDeferred::cleanup()
{
    if (_engine)
    {
        base::CpuProfileScope profile("renderer.waitIdle.cleanup");
        _engine->device()->waitIdle();
    }
    cleanupGIUpsampling();
    for (size_t i = _initializedOverlays; i > 0; --i)
    {
        _configuration.overlays.at(i - 1)->cleanup();
    }
    _initializedOverlays = 0;
    if (_temporalSurface)
    {
        _temporalSurface->cleanup();
        _temporalSurface.reset();
    }
    updateTemporalSurfaceStatus();
    if (_configuration.finalProcessor)
    {
        _configuration.finalProcessor->cleanup();
    }
    for (auto& selection : _configuration.layers)
    {
        selection.layer->cleanup();
    }
    if (_configuration.softShadows)
    {
        _configuration.softShadows->cleanup();
    }
    if (_configuration.ambientOcclusion)
    {
        _configuration.ambientOcclusion->cleanup();
    }
    if (_configuration.upscaler)
    {
        _configuration.upscaler->cleanup();
    }
    if (_fallback)
    {
        _fallback->cleanup();
        _fallback.reset();
    }
    _shadowConfiguration = {};
    _queryShadows = false;
    if (_configuration.globalIllumination)
    {
        _configuration.globalIllumination->cleanup();
    }
    if (_configuration.reflections)
    {
        _configuration.reflections->cleanup();
    }
    if (_configuration.temporal)
    {
        _configuration.temporal->cleanup();
    }
    if (_configuration.denoise)
    {
        _configuration.denoise->cleanup();
    }
    _historySignature.clear();
    _historyPolicySignature.clear();
    _historySignatureCurrent = false;
    _historyState.reset();
    _signatureCache.reset();
    _temporalCameraOwner.reset();
    _temporalCameraIdentity = nullptr;
    _ambientOcclusion = false;
    _globalIllumination = false;
    _reflections = false;
    _sharedNoise.reset();
    _noiseStatus = "No shared atlas allocated; effects use injected noise or hash";
    _frame.cleanup();
    _object.cleanup();
    _lights.cleanup();
    _environmentBinding.cleanup();
    _targets.clear();
    // cleanup's existing device drain precedes all correspondence releases. The
    // next lifecycle may restart tokens only after old frame snapshots are gone.
    _temporalCorrespondence.reset();
    _tracingResources.reset();
    _queue.clear();
    _drawLayout.reset();
    _lightingLayout.reset();
    _environment.reset();
    if (_generator)
    {
        _generator->cleanup();
        _generator.reset();
    }
    _configuration = {};
    _scene.reset();
    _environmentError.clear();
    _engine = nullptr;
    _initializationWarnings.clear();
}

std::shared_ptr<pipeline::RTGlobalIllumination> RendererDeferred::globalIllumination() const
{
    return _configuration.globalIllumination;
}

bool RendererDeferred::usesGlobalIllumination() const
{
    return _globalIllumination;
}

bool RendererDeferred::globalIlluminationEnabled() const
{
    return _globalIllumination && _configuration.globalIllumination->enabled();
}

IndirectLightingMode RendererDeferred::indirectLightingMode() const
{
    return _configuration.indirectLighting;
}

void RendererDeferred::setIndirectLightingMode(IndirectLightingMode value)
{
    if (uint32_t(value) > uint32_t(IndirectLightingMode::GlobalIllumination))
    {
        throw std::invalid_argument("Unknown indirect lighting mode");
    }
    _configuration.indirectLighting = value;
}

std::shared_ptr<pipeline::RTReflections> RendererDeferred::reflections() const
{
    return _configuration.reflections;
}

bool RendererDeferred::usesReflections() const
{
    return _reflections;
}

bool RendererDeferred::reflectionsEnabled() const
{
    return _reflections && _configuration.reflections->enabled();
}

std::shared_ptr<const TemporalSignatureState> RendererDeferred::prepareTemporalSignature(const RenderQueue& queue) const
{
    if (!_signatureCache)
    {
        _signatureCache = std::make_shared<TemporalSignatureCache>();
    }
    const bool correspondenceNeeded = _configuration.temporal->available() && _configuration.temporal->settings().enabled &&
        ((_ambientOcclusion && _configuration.ambientOcclusion->enabled() && _configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion) ||
         (_globalIllumination && _configuration.globalIllumination->enabled() && _configuration.indirectLighting == IndirectLightingMode::GlobalIllumination) ||
         (_reflections && _configuration.reflections->enabled()));
    return _signatureCache->prepare(queue, {_configuration, _projection, _explicitCamera, _temporalCameraIdentity,
        _temporalCameraOwner, _environment, correspondenceNeeded});
}

std::vector<uint8_t> RendererDeferred::fullTemporalSignature(const RenderQueue& queue) const
{
    base::CpuProfileScope profile("renderer.fullTemporalSignature");
    return serializeTemporalSignature(queue, true);
}

std::vector<uint8_t> RendererDeferred::historyPolicySignature(const RenderQueue& queue) const
{
    base::CpuProfileScope profile("renderer.historyPolicySignature");
    return serializeTemporalSignature(queue, false);
}

std::vector<uint8_t> RendererDeferred::serializeTemporalSignature(const RenderQueue& queue, bool includeTransforms) const
{
    std::vector<uint8_t> bytes;
    auto append = [&](const auto& value)
    {
        const auto* first = reinterpret_cast<const uint8_t*>(&value);
        bytes.insert(bytes.end(),first,first+sizeof(value));
    };
    auto pointer = [&](const auto* value)
    {
        append(reinterpret_cast<uintptr_t>(value));
    };
    append(_projection);
    append(_explicitCamera);
    pointer(_temporalCameraIdentity);
    append(_configuration.outputExtent.width);
    append(_configuration.outputExtent.height);
    append(_configuration.internalExtent.width);
    append(_configuration.internalExtent.height);
    append(_configuration.indirectLighting);
    pointer(_configuration.upscaler.get());
    const auto requirements = _configuration.upscaler->requirements();
    append(requirements.opaqueDepth);
    append(requirements.cameraMotion);
    append(requirements.jitter);
    append(requirements.storageOutput);
    const bool correspondenceNeeded = _configuration.temporal->available() && _configuration.temporal->settings().enabled &&
        ((_ambientOcclusion && _configuration.ambientOcclusion->enabled() && _configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion) ||
         (_globalIllumination && _configuration.globalIllumination->enabled() && _configuration.indirectLighting == IndirectLightingMode::GlobalIllumination) ||
         (_reflections && _configuration.reflections->enabled()));
    std::set<std::pair<std::string,uint32_t>> correspondence;
    for (const auto& item : queue.items())
    {
        if (correspondenceNeeded && (item.nodeId.empty() || !correspondence.emplace(item.nodeId,item.submeshIndex).second))
        {
            throw std::invalid_argument("Temporal correspondence requires unique nonempty node/submesh identities");
        }
        append(item.objectId);
        append(item.submeshIndex);
        append(item.nodeId.size());
        bytes.insert(bytes.end(),item.nodeId.begin(),item.nodeId.end());
        pointer(item.drawMesh.get());
        pointer(item.tracingMesh.get());
        pointer(item.drawMaterial.get());
        if (includeTransforms)
        {
            append(item.worldMatrix);
        }
        append(item.materialData);
        append(item.rayTracingEnabled);
        for (uint32_t i = 0; i < 6; ++i)
        {
            pointer(item.textureImages[i]);
            pointer(item.textureSamplers[i]);
        }
    }
    append(queue.lights());
    pointer(_environment.get());
    if (_environment)
    {
        pointer(_environment->sky.get());
        pointer(_environment->irradiance.get());
        pointer(_environment->prefilteredSpecular.get());
        pointer(_environment->brdf.get());
        pointer(_environment->sampler.get());
        append(_environment->skyboxBlur);
        append(_environment->skyboxVisible);
    }
    auto noise = [&](const auto& owner)
    {
        pointer(owner.get());
        if (owner && owner->isReady())
        {
            pointer(owner->image().get());
            pointer(owner->sampler().get());
            append(owner->shaderMetadata());
        }
    };
    const auto& ao = _configuration.ambientOcclusion->settings();
    append(ao.enabled);
    append(ao.quality);
    append(ao.sampleCount);
    append(ao.bounceCount);
    append(ao.radius);
    append(ao.bias);
    append(ao.falloff);
    append(ao.bounceAttenuation);
    append(ao.useBlueNoise);
    noise(_configuration.ambientOcclusion->noise());
    const auto& gi = _configuration.globalIllumination->settings();
    append(gi.enabled);
    append(gi.quality);
    append(gi.sampleCount);
    append(gi.bounceCount);
    append(gi.bias);
    append(gi.maxDistance);
    append(gi.shadowSamples);
    append(gi.useShadows);
    append(gi.useBlueNoise);
    noise(_configuration.globalIllumination->noise());
    const auto& reflection = _configuration.reflections->settings();
    append(reflection.enabled);
    append(reflection.quality);
    append(reflection.sampleCount);
    append(reflection.bias);
    append(reflection.maxDistance);
    append(reflection.maxRoughness);
    append(reflection.roughnessSpread);
    append(reflection.shadowSamples);
    append(reflection.useShadows);
    append(reflection.useBlueNoise);
    noise(_configuration.reflections->noise());
    const auto& shadows = _configuration.softShadows->settings();
    append(shadows.enabled);
    append(shadows.bias);
    append(shadows.directionalDistance);
    append(shadows.maxSamples);
    const auto& temporal = _configuration.temporal->settings();
    append(temporal.enabled);
    append(temporal.maxFrames);
    append(temporal.freezeAtLimit);
    append(temporal.absolutePositionThreshold);
    append(temporal.relativePositionThreshold);
    append(temporal.normalThreshold);
    append(temporal.adaptiveEnabled);
    append(temporal.minCurrentWeight);
    append(temporal.reactiveCurrentWeight);
    append(temporal.historyClipSigma);
    // Use virtual getters so injected filter controls report their effective values.
    append(_configuration.denoise->enabled());
    append(_configuration.denoise->kernelRadius());
    append(_configuration.denoise->depthThreshold());
    append(_configuration.denoise->normalThreshold());
    append(_configuration.denoise->depthSigma());
    append(_configuration.denoise->normalSigma());
    append(_configuration.denoise->spatialSigma());
    base::CpuProfileScope::count(includeTransforms ? "temporal.fullSignatureBytes" : "temporal.policySignatureBytes", bytes.size(), "bytes");
    return bytes;
}

std::shared_ptr<pipeline::DenoiseFilter> RendererDeferred::denoise() const
{
    return _configuration.denoise;
}

std::shared_ptr<pipeline::FinalProcessor> RendererDeferred::finalProcessor() const
{
    return _configuration.finalProcessor;
}

const std::string& RendererDeferred::noiseStatus() const
{
    return _noiseStatus;
}

}
