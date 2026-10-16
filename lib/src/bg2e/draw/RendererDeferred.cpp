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
    if (count == 0 && engine.surface())
    {
        count = engine.surface()->inFlightFrames();
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
    // An explicitly injected unshadowed lighting contract opts this graph out
    // without forcing an incompatible replacement on query-capable devices.
    const bool unshadowedReplacement = std::any_of(_configuration.layers.begin(), _configuration.layers.end(), [](const auto& selection)
    {
        return selection.configuration.contract.shaderInterface == "draw.raster.deferred.v1";
    });
    _queryShadows = _configuration.softShadows->available() && !unshadowedReplacement;
    const bool queryShadows = _queryShadows;
    auto supplied = std::move(_configuration.layers);
    _configuration.layers.clear();
    auto sky = std::make_shared<environment::SkyboxRenderer>();
    sky->settings().shaderRoot = _shaderRoot;
    std::vector<std::shared_ptr<pipeline::RenderLayer>> defaults{
        std::make_shared<pipeline::GBuffer>(_shaderRoot),
        std::make_shared<pipeline::DeferredLayer>(_shaderRoot, count, depth, queryShadows),
        std::make_shared<pipeline::FinalLayer>(_shaderRoot, count), sky};
    const auto hdr = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    std::vector<pipeline::PassContract> contracts{pipeline::gbufferContract(depth), pipeline::deferredContract(depth, queryShadows),
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
    _lightingLayout = engine.device()->createPipelineLayout(pipeline::deferredBindingLayout(queryShadows));
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
        output.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
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
            _view = inputs.camera ? inputs.camera->viewMatrix() : glm::mat4{1};
            _projection = inputs.camera ? inputs.camera->projectionMatrix() : glm::mat4{1};
        }
        RenderQueueVisitor visitor;
        visitor.enqueue(_scene.get(), *_engine, slot.queue);
    }
    else
    {
        slot.queue.beginFrame(*_engine);
    }
    slot.queue.sort(glm::vec3(glm::inverse(_view)[3]));
    _queue = slot.queue;
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
}

void RendererDeferred::dispatch(size_t stage, const FrameContext& frame,
    const std::vector<pipeline::ImageBinding>& inputs, const std::vector<pipeline::ImageBinding>& outputs,
    const pipeline::RasterFrame* raster)
{
    const auto& selected = _configuration.layers.at(stage);
    Renderer::recordLayer(selected, {frame, selected.configuration, inputs, outputs, _scene.get(), raster});
}

void RendererDeferred::record(const FrameContext& frame)
{
    checkFrame(frame);
    auto& slot = _targets[frame.frameSlot];
    if (!slot.prepared || slot.serial != frame.frameNumber)
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
    auto composite = [&](gpu::Image* background, gpu::Image* output)
    {
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
        dispatch(1, frame, inputs, {{"color", output}}, &raster);
    };
    composite(slot.colorA.get(), slot.colorB.get());
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
        composite(current, next);
        std::swap(current, next);
    }
    UpscalerInput upscale{frame, *current, *slot.opaqueDepth, *slot.reconstructed, slot.camera, slot.jitteredProjection};
    if (!_configuration.upscaler->available() || !_configuration.upscaler->record(upscale))
    {
        _fallback->record(upscale);
    }
    std::vector<pipeline::ImageBinding> finalInputs{{"color", slot.reconstructed.get()}};
    finalInputs.insert(finalInputs.end(), gbuffer.begin(), gbuffer.end());
    dispatch(2, frame, finalInputs, {{"output", &frame.colorTarget}}, &raster);
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
    return _configuration.upscaler && _configuration.upscaler->needsRebuild();
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
    if (_configuration.upscaler)
    {
        _configuration.upscaler->reset();
    }
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
    for (auto& selection : _configuration.layers)
    {
        selection.layer->cleanup();
    }
    if (_configuration.softShadows)
    {
        _configuration.softShadows->cleanup();
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
}
