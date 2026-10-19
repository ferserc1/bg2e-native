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
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <algorithm>
#include <cmath>
#include <set>

namespace bg2e::draw {
namespace {
class SceneInputs : public scene::NodeVisitor {
  public:
    SceneInputs()
    {
        setIgnoreDisabled(true);
    }

    scene::CameraComponent* camera = nullptr;
    scene::EnvironmentComponent* environment = nullptr;

    void visit(scene::Node* node) override
    {
        if (!camera)
        {
            camera = node->camera();
        }
        if (!environment)
        {
            environment = node->getComponent<scene::EnvironmentComponent>();
        }
    }
};
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
    _configuration.upscaler->initialize(engine, upscaleConfiguration());
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
        _configuration.finalProcessor->initialize(engine,_configuration.outputFormat,count);
    }
    const auto ao = _configuration.ambientOcclusion;
    const auto gi = _configuration.globalIllumination;
    const auto reflection = _configuration.reflections;
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
    _drawLayout = engine.device()->createPipelineLayout(drawBindingLayout());
    _lightingLayout = engine.device()->createPipelineLayout(pipeline::deferredBindingLayout(queryShadows, _ambientOcclusion, _globalIllumination, _reflections));
    _frame.initialize(engine, _lightingLayout, count);
    _object.initialize(engine, _drawLayout, count);
    _lights.initialize(engine, _lightingLayout, count);
    _environmentBinding.initialize(engine, _lightingLayout, count, 8);
    for (const auto& overlay : _configuration.overlays)
    {
        ++_initializedOverlays; // Cleanup also covers partially initialized overlays.
        overlay->initialize(engine, overlayConfiguration());
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
        _generator->synchronize(nullptr);
        _environment = _generator->resources();
    }
    std::dynamic_pointer_cast<environment::SkyboxPass>(_configuration.layers[3].layer)->setEnvironment(_environment);
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
    _engine->device()->waitIdle();
    _scene = std::move(sceneRoot);
    _queue.clear();
    resetUpscaler();
    if (!_explicitCamera)
    {
        _view = _projection = glm::mat4{1};
    }
    for (auto& slot : _targets)
    {
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
    _engine->device()->waitIdle();
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
    if (!_engine)
    {
        return false;
    }
    bool overlayChanged = false;
    for (const auto& overlay : _configuration.overlays)
    {
        overlayChanged = overlay->prepareFrame() || overlayChanged;
    }
    if (!_generator)
    {
        return overlayChanged;
    }
    SceneInputs inputs;
    if (_scene)
    {
        _scene->accept(&inputs);
    }
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
    checkFrame(frame);
    auto& slot = _targets[frame.frameSlot];
    if (slot.prepared && slot.serial == frame.frameNumber)
    {
        throw std::logic_error("Renderer updates once per acquired presentation");
    }
    if (_scene)
    {
        scene::UpdateVisitor update;
        update.update(_scene.get(), frame.sceneDeltaSeconds);
        SceneInputs inputs;
        _scene->accept(&inputs);
        if (!_explicitCamera)
        {
            _temporalCameraIdentity = inputs.camera;
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
    }
    slot.queue.sort(glm::vec3(glm::inverse(_view)[3]));
    _queue = slot.queue;
    auto signature = temporalSignature(slot.queue);
    if (signature != _historySignature)
    {
        invalidateHistories();
        _historySignature = std::move(signature);
    }
    _configuration.temporal->beginFrame(frame.frameSlot,frame.frameNumber,frame.sceneFrameNumber);
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
    _object.prepare(frame, slot.queue);
    _lights.update(frame, slot.queue.lights());
    _environmentBinding.update(frame, _environment);
    if (_queryShadows)
    {
        _configuration.softShadows->prepare(frame, slot.queue, _view, slot.jitteredProjection);
    }
    if (_ambientOcclusion)
    {
        if (_configuration.indirectLighting == IndirectLightingMode::AmbientOcclusion)
        {
            _configuration.ambientOcclusion->prepare(frame.frameSlot,frame.frameNumber,slot.queue);
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
            _configuration.globalIllumination->prepare(frame.frameSlot,frame.frameNumber,slot.queue);
        }
        else
        {
            _configuration.globalIllumination->prepareSnapshot(frame.frameSlot,frame.frameNumber,nullptr);
        }
    }
    if (_reflections)
    {
        _configuration.reflections->prepare(frame.frameSlot,frame.frameNumber,slot.queue);
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
    checkFrame(frame);
    auto& slot = _targets[frame.frameSlot];
    if (!slot.prepared || slot.serial != frame.frameNumber || slot.linearRecorded)
    {
        throw std::logic_error("Update the acquired renderer slot before recording");
    }
    pipeline::RasterFrame raster{&slot.queue, nullptr, &_frame, &_object, &_lights, &_environmentBinding};
    std::vector<pipeline::ImageBinding> gbuffer;
    const auto& contract = _configuration.layers[0].configuration.contract;
    for (size_t i = 0; i < contract.outputs.size(); ++i)
    {
        gbuffer.push_back({contract.outputs[i].semantic, slot.gbuffer.at(i).get()});
    }
    dispatch(0, frame, {}, gbuffer, &raster);
    pipeline::RenderTarget::copy(frame.commandBuffer, *slot.gbuffer[6], *slot.opaqueDepth);
    pipeline::RenderTarget::clear(frame.commandBuffer, *slot.colorA);
    dispatch(3, frame, {}, {{"color", slot.colorA.get()}, {"depth", slot.opaqueDepth.get()}});
    auto composite = [&](gpu::Image* background, gpu::Image* output, uint64_t layer)
    {
        auto resolveSignal = [&](pipeline::TemporalSignal kind, gpu::Image& signal, gpu::Image* material = nullptr,
            float maxRoughness = 1.0f)
        {
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = slot.gbuffer[6].get();
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*_view);
            const pipeline::TemporalView view{_view,_projection,slot.jitteredProjection,true};
            const auto accumulated = _configuration.temporal->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                frame.sceneFrameNumber,_historyGeneration,{kind,0,layer},signal,surface,view,material,maxRoughness});
            pipeline::DenoiseSignal filteredKind = pipeline::DenoiseSignal::Visibility;
            if (kind == pipeline::TemporalSignal::DiffuseRadiance)
            {
                filteredKind = pipeline::DenoiseSignal::DiffuseRadiance;
            }
            else if (kind == pipeline::TemporalSignal::ReflectedRadiance)
            {
                filteredKind = pipeline::DenoiseSignal::ReflectedRadiance;
            }
            return _configuration.denoise->record({frame.commandBuffer,frame.frameSlot,frame.frameNumber,
                {filteredKind,0,layer},*accumulated.image,surface,_view,material,maxRoughness}).image;
        };
        auto inputs = gbuffer;
        inputs.push_back({"background", background});
        if (_queryShadows)
        {
            std::vector<pipeline::ImageBinding> visibility{{"shadow0", slot.shadows[0].get()}, {"shadow1", slot.shadows[1].get()}};
            const std::vector<pipeline::ImageBinding> sources{{"normalEmission", slot.gbuffer[1].get()}, {"depth", slot.gbuffer[6].get()}};
            pipeline::LayerSelection selection{_configuration.softShadows, _shadowConfiguration};
            Renderer::recordLayer(selection, {frame, selection.configuration, sources, visibility, _scene.get(), &raster});
            inputs.insert(inputs.end(), visibility.begin(), visibility.end());
        }
        if (_ambientOcclusion)
        {
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = slot.gbuffer[6].get();
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*_view);
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
            if (_configuration.indirectLighting == IndirectLightingMode::GlobalIllumination)
            {
                pipeline::SurfaceInputs surface;
                surface.normal = slot.gbuffer[1].get();
                surface.depthOrPosition = slot.gbuffer[6].get();
                surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*_view);
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
                signal = resolveSignal(pipeline::TemporalSignal::DiffuseRadiance,*signal);
            }
            inputs.push_back({"globalIllumination",signal});
        }
        if (_reflections)
        {
            pipeline::SurfaceInputs surface;
            surface.normal = slot.gbuffer[1].get();
            surface.depthOrPosition = slot.gbuffer[6].get();
            surface.inverseViewProjection = glm::inverse(slot.jitteredProjection*_view);
            const auto camera = glm::vec3(glm::inverse(_view)[3]);
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
        dispatch(1, frame, inputs, {{"color", output}}, &raster);
    };
    composite(slot.colorA.get(), slot.colorB.get(),0);
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
    const glm::vec3 camera(glm::inverse(_view)[3]);
    std::stable_sort(transparent.begin(), transparent.end(), [&](const auto* a, const auto* b)
    {
        const auto da = glm::vec3(a->worldMatrix[3]) - camera;
        const auto db = glm::vec3(b->worldMatrix[3]) - camera;
        return glm::dot(da, da) > glm::dot(db, db);
    });
    for (const auto* item : transparent)
    {
        if (_inspectOpaqueLayer)
        {
            break;
        }
        pipeline::RenderTarget::copy(frame.commandBuffer, *slot.opaqueDepth, *slot.gbuffer[6]);
        raster.item = item;
        dispatch(0, frame, {}, gbuffer, &raster);
        composite(current, next,(uint64_t(item->objectId)<<32)|item->submeshIndex);
        std::swap(current, next);
    }
    slot.finalItem = raster.item;
    UpscalerInput upscale{frame, *current, *slot.opaqueDepth, *slot.reconstructed, slot.camera, slot.jitteredProjection};
    if (!_configuration.upscaler->available() || !_configuration.upscaler->record(upscale))
    {
        _fallback->record(upscale);
    }
    frame.commandBuffer.transition(slot.reconstructed.get(), gpu::ImageLayout::ShaderReadOnly);
    slot.linearRecorded = true;
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
    auto result = temporalSignature(_queue);
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
    if (_historyGeneration == UINT64_MAX)
    {
        throw std::overflow_error("Renderer history generation exhausted");
    }
    ++_historyGeneration;
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
        _engine->device()->waitIdle();
    }
    for (size_t i = _initializedOverlays; i > 0; --i)
    {
        _configuration.overlays.at(i - 1)->cleanup();
    }
    _initializedOverlays = 0;
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
}

std::shared_ptr<pipeline::RTGlobalIllumination> RendererDeferred::globalIllumination() const
{
    return _configuration.globalIllumination;
}

bool RendererDeferred::usesGlobalIllumination() const
{
    return _globalIllumination;
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

std::vector<uint8_t> RendererDeferred::temporalSignature(const RenderQueue& queue) const
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
        append(item.worldMatrix);
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
    // Use virtual getters so injected filter controls report their effective values.
    append(_configuration.denoise->enabled());
    append(_configuration.denoise->kernelRadius());
    append(_configuration.denoise->depthThreshold());
    append(_configuration.denoise->normalThreshold());
    append(_configuration.denoise->depthSigma());
    append(_configuration.denoise->normalSigma());
    append(_configuration.denoise->spatialSigma());
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
