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

#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/FsrUpscaler.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <typeinfo>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>

#include <exception>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace {
std::shared_ptr<pipeline::FinalLayer> finalColorTarget(const Renderer* renderer)
{
    if (!renderer || typeid(*renderer) != typeid(RendererDeferred))
    {
        return {};
    }
    const auto& configuration = renderer->configuration();
    if (configuration.layers.size() <= 2)
    {
        return {};
    }
    const auto* layer = configuration.layers[2].layer.get();
    const auto* temporal = configuration.temporal.get();
    const auto* upscaler = configuration.upscaler.get();
    // History invalidation remains part of settings edits. Unknown virtual reset
    // implementations cannot inherit the proof for the built-in logical resets.
    if (!layer || typeid(*layer) != typeid(pipeline::FinalLayer) ||
        (temporal && typeid(*temporal) != typeid(pipeline::TemporalAccumulator)) ||
        (upscaler && typeid(*upscaler) != typeid(ResamplingUpscaler) && typeid(*upscaler) != typeid(FsrUpscaler)))
    {
        return {};
    }
    return std::static_pointer_cast<pipeline::FinalLayer>(configuration.layers[2].layer);
}
}

DefaultRenderLoopDelegate::DefaultRenderLoopDelegate(std::unique_ptr<Renderer> renderer,
                                                   RendererConfiguration configuration)
{
    setRenderer(std::move(renderer), std::move(configuration));
}

std::shared_ptr<scene::Node> DefaultRenderLoopDelegate::createScene()
{
    return std::make_shared<scene::Node>();
}

void DefaultRenderLoopDelegate::init(Engine* engine)
{
    if (_engine || !engine)
    {
        throw std::logic_error("Default draw delegate requires one initialized engine");
    }
    engine->device();
    RenderLoopDelegate::init(engine);
}

void DefaultRenderLoopDelegate::initScene()
{
    if (!_engine || _sceneReady)
    {
        throw std::logic_error("Default draw scene initialization is out of order");
    }
    _scene = _scenePending ? std::move(_pendingScene) : createScene();
    _scenePending = false;
    _sceneReady = true;
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setRenderer(std::unique_ptr<Renderer> renderer,
                                          RendererConfiguration configuration)
{
    if (!renderer)
    {
        throw std::invalid_argument("Default draw delegate requires a non-null renderer");
    }
    _pendingRenderer = PendingRenderer{std::move(renderer), std::move(configuration)};
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setScene(std::shared_ptr<scene::Node> sceneRoot)
{
    _pendingScene = std::move(sceneRoot);
    _scenePending = true;
    requestSceneFrame();
}

std::shared_ptr<scene::Node> DefaultRenderLoopDelegate::scene() const
{
    return _scene;
}

Renderer* DefaultRenderLoopDelegate::renderer() const
{
    return _renderer.get();
}

uint64_t DefaultRenderLoopDelegate::rendererGeneration() const
{
    return _rendererGeneration;
}

bool DefaultRenderLoopDelegate::withRenderer(uint64_t generation, const std::function<void(Renderer&)>& operation)
{
    if (!_targetsBound || generation != _rendererGeneration || !operation)
    {
        return false;
    }
    operation(*_renderer);
    return true;
}

void DefaultRenderLoopDelegate::setRendererChangeCallbacks(std::function<void()> detach,
                                                         RendererAttachedCallback attach)
{
    if (_targetsBound)
    {
        throw std::logic_error("Configure renderer callbacks before targets are bound");
    }
    _detach = std::move(detach);
    _attach = std::move(attach);
}

void DefaultRenderLoopDelegate::setContinuousRendering(bool enabled)
{
    _continuousRendering = enabled;
    if (enabled)
    {
        requestSceneFrame();
    }
}

bool DefaultRenderLoopDelegate::continuousRendering() const
{
    return _continuousRendering;
}

void DefaultRenderLoopDelegate::detachTargets()
{
    if (_targetsBound)
    {
        base::CpuProfileScope profile("delegate.detachTargetsSettingsBinding");
        _targetsBound = false;
        ++_rendererGeneration; // Invalidate delayed actions before calling user code.
        const auto detach = _detach;
        base::CpuProfileScope::count("delegate.settingsDetachCallbacks", detach ? 1 : 0);
        if (detach)
        {
            detach();
        }
    }
}

void DefaultRenderLoopDelegate::attachTargets()
{
    base::CpuProfileScope profile("delegate.attachTargetsSettingsBinding");
    base::CpuProfileScope::count("delegate.settingsAttachCallbacks", _attach ? 1 : 0);
    ++_rendererGeneration;
    _targetsBound = true;
    const auto attach = _attach;
    if (attach)
    {
        attach(*_renderer, _rendererGeneration);
    }
}

void DefaultRenderLoopDelegate::releaseRenderer()
{
    std::exception_ptr error;
    try
    {
        detachTargets();
    }
    catch (...)
    {
        error = std::current_exception();
    }
    if (_rendererInitialized)
    {
        _rendererInitialized = false;
        try
        {
            _renderer->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    }
    _renderer.reset();
    _initialConfiguration = {};
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void DefaultRenderLoopDelegate::initializeRenderer()
{
    if (!_renderer || _rendererInitialized || !_sceneReady || _outputExtent.isZero())
    {
        return;
    }
    auto configuration = std::move(_initialConfiguration);
    configuration.outputExtent = _outputExtent;
    configuration.outputFormat = _outputFormat;
    if (_followOutputExtent)
    {
        configuration.internalExtent = _outputExtent;
    }
    if (configuration.resolution)
    {
        configuration.internalExtent = configuration.resolution->resolve(_outputExtent, configuration.internalExtent);
    }
    for (auto& layer : configuration.layers)
    {
        layer.configuration.outputExtent = configuration.outputExtent;
        layer.configuration.internalExtent = configuration.internalExtent;
    }
    Renderer::validateConfiguration(configuration);
    _rendererInitialized = true; // Cleanup must cover partial initialization too.
    try
    {
        _renderer->initialize(*_engine, std::move(configuration));
        _renderer->attachScene(_scene);
        attachTargets();
        // Attachment callbacks may restore resolution/effect quality. Rebuild
        // before the first record, including surfaceChanged after acquisition.
        reconfigureRenderer();
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            releaseRenderer();
        }
        catch (...)
        {
        }
        std::rethrow_exception(error);
    }
}

void DefaultRenderLoopDelegate::enqueueRendererUpdate(uint64_t generation,
    std::function<void(Renderer&)> operation, std::function<void()> stale)
{
    if (!operation)
    {
        throw std::invalid_argument("Renderer update requires an operation");
    }
    PendingUpdate update;
    update.generation = generation;
    update.operation = std::move(operation);
    update.stale = std::move(stale);
    _updates.push_back(std::move(update));
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::enqueueCpuValueUpdate(uint64_t generation,
    std::function<void(Renderer&)> operation, std::function<void()> stale)
{
    if (!operation)
    {
        throw std::invalid_argument("CPU value update requires an operation");
    }
    PendingUpdate update;
    update.generation = generation;
    update.operation = std::move(operation);
    update.stale = std::move(stale);
    update.cpuValuesOnly = true;
    _updates.push_back(std::move(update));
    requestSceneFrame();
}


void DefaultRenderLoopDelegate::enqueueRetainedResourceUpdate(uint64_t generation,
    std::function<void(Renderer&)> operation, std::function<void()> stale)
{
    if (!operation)
    {
        throw std::invalid_argument("Retained resource update requires an operation");
    }
    PendingUpdate update;
    update.generation = generation;
    update.operation = std::move(operation);
    update.stale = std::move(stale);
    update.retainsResources = true;
    _updates.push_back(std::move(update));
    requestSceneFrame();
}


bool DefaultRenderLoopDelegate::enqueueFinalColorUpdate(uint64_t generation,
    FinalColorProperty property, float value, std::function<void(ValueUpdateResult)> completion, std::function<bool()> current)
{
    auto target = _targetsBound ? finalColorTarget(_renderer.get()) : nullptr;
    if (!target)
    {
        return false;
    }
    // Match the four standard setters, before publishing a queue entry.
    float minimum = 0, maximum = 0;
    switch (property)
    {
    case FinalColorProperty::Exposure: minimum = -32; maximum = 32; break;
    case FinalColorProperty::Contrast: minimum = 0; maximum = 10; break;
    case FinalColorProperty::Brightness: minimum = -1; maximum = 1; break;
    case FinalColorProperty::Gamma: minimum = 0.1f; maximum = 8; break;
    default: throw std::invalid_argument("Unknown final color property");
    }
    if (!std::isfinite(value) || value < minimum || value > maximum)
    {
        throw std::invalid_argument("Final color value is outside its supported range");
    }
    // Coalesce only adjacent unobserved assignments to the same target/property.
    // Every callback, different property and generic operation is an order barrier.
    if (!completion && !current && !_updates.empty())
    {
        auto& previous = _updates.back();
        if (previous.color && !previous.completion && !previous.current && previous.generation == generation &&
            previous.color->property == property && previous.color->target.lock() == target)
        {
            previous.color->value = value;
            base::CpuProfileScope::count("delegate.valueUpdatesCoalesced", 1);
            requestSceneFrame();
            return true;
        }
    }
    PendingUpdate update;
    update.generation = generation;
    update.color = PendingUpdate::ColorValue{property, value, target};
    update.completion = std::move(completion);
    update.current = std::move(current);
    _updates.push_back(std::move(update));
    requestSceneFrame();
    return true;
}

void DefaultRenderLoopDelegate::applyUpdates(std::vector<PendingUpdate> updates)
{
    if (updates.empty())
    {
        return;
    }
    base::CpuProfileScope::count("delegate.queuedRendererUpdateBatch", updates.size());
    const bool conservative = !_valueUpdateDrainElisionEnabled || std::any_of(updates.begin(), updates.end(), [](const auto& update)
    {
        return !update.color && !update.cpuValuesOnly && !update.retainsResources;
    });
    std::optional<gpu::Device::ResourceUpdateScope> resourceUpdate;
    if (conservative)
    {
        base::CpuProfileScope profile("delegate.resourceSync.queuedRendererUpdates");
        base::CpuProfileScope::count("delegate.queuedRendererUpdateSyncRequests", 1, "requests");
        resourceUpdate.emplace(*_engine->device());
    }
    base::CpuProfileScope updateProfile("delegate.applyQueuedRendererUpdates");
    const bool valueOnly = std::all_of(updates.begin(), updates.end(), [](const auto& update)
    {
        return update.color || update.cpuValuesOnly;
    });
    base::CpuProfileScope::count("delegate.valueOnlyUpdateBatches", valueOnly && !conservative ? 1 : 0);
    for (auto& update : updates)
    {
        if (update.color)
        {
            base::CpuProfileScope valueProfile("delegate.applyFinalColorValue");
            base::CpuProfileScope::count("delegate.valueUpdates", 1);
            const auto target = update.color->target.lock();
            const bool current = _targetsBound && update.generation == _rendererGeneration &&
                target && finalColorTarget(_renderer.get()) == target && (!update.current || update.current());
            auto result = ValueUpdateResult::Stale;
            if (current)
            {
                auto proposed = target->settings;
                switch (update.color->property)
                {
                case FinalColorProperty::Exposure: proposed.color.x = update.color->value; break;
                case FinalColorProperty::Contrast: proposed.color.y = update.color->value; break;
                case FinalColorProperty::Brightness: proposed.color.z = update.color->value; break;
                case FinalColorProperty::Gamma: proposed.color.w = update.color->value; break;
                }
                // Compare explicit float components, preserving signed-zero bits.
                bool changed = false;
                for (int i = 0; i < 4; ++i)
                {
                    changed = changed || std::memcmp(&proposed.color[i], &target->settings.color[i], sizeof(float)) != 0;
                }
                if (changed)
                {
                    target->settings.color = proposed.color;
                    _renderer->invalidateHistories();
                    requestSceneFrame();
                    result = ValueUpdateResult::Applied;
                }
                else
                {
                    result = ValueUpdateResult::Unchanged;
                }
            }
            base::CpuProfileScope::count("delegate.valueUpdatesApplied", result == ValueUpdateResult::Applied ? 1 : 0);
            base::CpuProfileScope::count("delegate.valueUpdatesUnchanged", result == ValueUpdateResult::Unchanged ? 1 : 0);
            base::CpuProfileScope::count("delegate.valueUpdatesStale", result == ValueUpdateResult::Stale ? 1 : 0);
            if (update.completion)
            {
                update.completion(result);
            }
            continue;
        }
        base::CpuProfileScope::count(update.cpuValuesOnly ? "delegate.cpuValueUpdates" :
            update.retainsResources ? "delegate.retainedResourceUpdates" : "delegate.conservativeUpdates", 1);
        if (!withRenderer(update.generation, update.operation))
        {
            if (update.stale)
            {
                update.stale();
            }
        }
        else
        {
            requestSceneFrame();
        }
    }
}

void DefaultRenderLoopDelegate::prepareFrame()
{
    if (!_engine || !_sceneReady)
    {
        throw std::logic_error("Default draw delegate scene is not initialized");
    }
    base::CpuProfileScope prepareProfile("delegate.prepareFrame");
    base::CpuProfileScope::count("delegate.prepareFrameCalls", 1);
    auto updates = std::move(_updates);
    _updates.clear(); // Updates queued by callbacks wait for the next prepare hook.
    base::CpuProfileScope::count("delegate.pendingRendererUpdates", updates.size());
    base::CpuProfileScope::count("delegate.rendererReplacementPending", _pendingRenderer ? 1 : 0);
    base::CpuProfileScope::count("delegate.sceneReplacementPending", _scenePending ? 1 : 0);
    if (!_pendingRenderer && !_scenePending)
    {
        base::CpuProfileScope stageProfile("delegate.applyQueuedUpdatesAndReconfigure");
        applyUpdates(std::move(updates));
        reconfigureRenderer();
        stageProfile.next("delegate.rendererPrepareFrame");
        if (_rendererInitialized && _renderer->prepareFrame())
        {
            requestSceneFrame();
        }
        return;
    }
    // This hook runs before acquisition or UI callbacks. Background producers
    // remain the application's responsibility and must not submit concurrently.
    std::optional<gpu::Device::ResourceUpdateScope> resourceUpdate;
    {
        base::CpuProfileScope profile(_pendingRenderer
            ? (_scenePending ? "delegate.resourceSync.rendererAndSceneReplacement" : "delegate.resourceSync.rendererReplacement")
            : "delegate.resourceSync.sceneReplacement");
        base::CpuProfileScope::count("delegate.replacementSyncRequests", 1, "requests");
        resourceUpdate.emplace(*_engine->device());
    }
    base::CpuProfileScope stageProfile("delegate.commitReplacement");
    // Snapshot before callbacks: replacements queued by detach/attach run later.
    auto pending = std::move(_pendingRenderer);
    _pendingRenderer.reset();
    const bool scenePending = _scenePending;
    auto sceneRoot = scenePending ? std::move(_pendingScene) : _scene;
    _scenePending = false;
    if (pending)
    {
        stageProfile.next("delegate.releaseRenderer");
        releaseRenderer();
        stageProfile.next("delegate.installRendererReplacement");
        _renderer = std::move(pending->renderer);
        _initialConfiguration = std::move(pending->configuration);
        _followOutputExtent = _initialConfiguration.internalExtent.isZero();
        _fixedInternalExtent = _initialConfiguration.internalExtent;
    }
    else
    {
        stageProfile.next("delegate.detachSceneTargets");
        detachTargets();
    }
    _scene = std::move(sceneRoot);
    if (_rendererInitialized)
    {
        stageProfile.next("delegate.attachScene");
        _renderer->attachScene(_scene);
        stageProfile.next("delegate.attachTargets");
        attachTargets();
    }
    else
    {
        stageProfile.next("delegate.initializeRenderer");
        initializeRenderer();
    }
    stageProfile.next("delegate.applyQueuedUpdates");
    applyUpdates(std::move(updates));
    reconfigureRenderer();
    stageProfile.next("delegate.sceneVisitorUpdate");
    // The new scene/resources are attached at this safe boundary. Give its
    // update visitor one pass before asking the renderer to synchronize
    // scene-derived resources such as its environment map.
    if (_scene)
    {
        scene::UpdateVisitor update;
        update.update(_scene.get(), 0.0f);
    }
    stageProfile.next("delegate.rendererPrepareFrame");
    if (_rendererInitialized && _renderer->prepareFrame())
    {
        requestSceneFrame();
    }
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setInternalExtent(gpu::Size2D extent)
{
    if ((extent.width == 0) != (extent.height == 0))
    {
        throw std::invalid_argument("Internal extent must have both dimensions or follow output");
    }
    _followOutputExtent = extent.isZero();
    _fixedInternalExtent = extent;
    _initialConfiguration.internalExtent = extent;
    reconfigureRenderer();
}

void DefaultRenderLoopDelegate::reconfigureRenderer()
{
    if (!_rendererInitialized)
    {
        return;
    }
    const auto internal = _renderer->resolveInternalExtent(_outputExtent,
        _followOutputExtent ? _outputExtent : _fixedInternalExtent);
    if (internal != _renderer->configuration().internalExtent || _renderer->needsReconfiguration())
    {
        base::CpuProfileScope reconfigureProfile("delegate.reconfigureRenderer");
        {
            base::CpuProfileScope profile("delegate.resourceSync.rendererReconfiguration");
            base::CpuProfileScope::count("delegate.reconfigurationSyncRequests", 1, "requests");
            _engine->device()->waitIdleForResourceUpdate();
        }
        detachTargets();
        _renderer->resize(_outputExtent, internal);
        attachTargets();
        requestSceneFrame();
    }
}

void DefaultRenderLoopDelegate::surfaceChanged(gpu::Size2D extent, gpu::PixelFormat format)
{
    if (!_engine || !_sceneReady || extent.isZero() || format == gpu::PixelFormat::Undefined || gpu::isDepthFormat(format))
    {
        throw std::invalid_argument("Default draw delegate requires a valid resolved color surface");
    }
    const bool formatChanged = _outputFormat != format;
    _outputExtent = extent;
    _outputFormat = format;
    if (_rendererInitialized)
    {
        // RenderLoop drained all users before notifying this callback.
        detachTargets();
        if (formatChanged)
        {
            _initialConfiguration = _renderer->configuration();
            _initialConfiguration.internalExtent = _followOutputExtent ? gpu::Size2D{} : _fixedInternalExtent;
            _rendererInitialized = false;
            _renderer->cleanup();
            initializeRenderer();
            requestSceneFrame();
            return;
        }
        _renderer->resize(extent, _renderer->resolveInternalExtent(extent,
            _followOutputExtent ? extent : _fixedInternalExtent));
        attachTargets();
    }
    else
    {
        initializeRenderer();
    }
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::resize(gpu::Size2D extent)
{
    if (!_engine)
    {
        throw std::logic_error("Default draw delegate is not initialized");
    }
    surfaceChanged(extent, _engine->renderSurface()->colorFormat());
}

void DefaultRenderLoopDelegate::update(const FrameContext& frame)
{
    if (!_rendererInitialized || !_targetsBound)
    {
        throw std::logic_error("Default draw renderer is not ready");
    }
    _renderer->update(frame);
    if (_continuousRendering)
    {
        requestSceneFrame();
    }
}

void DefaultRenderLoopDelegate::render(const FrameContext& frame)
{
    if (!_rendererInitialized || !_targetsBound)
    {
        throw std::logic_error("Default draw renderer is not ready");
    }
    _renderer->record(frame);
}

void DefaultRenderLoopDelegate::cleanup()
{
    std::exception_ptr error;
    // RenderLoop has already drained GPU users. Direct callers must do so too.
    try
    {
        releaseRenderer();
    }
    catch (...)
    {
        error = std::current_exception();
    }
    _updates.clear();
    _pendingRenderer.reset();
    _pendingScene.reset();
    _scene.reset();
    _scenePending = false;
    _sceneReady = false;
    _outputExtent = {};
    _outputFormat = gpu::PixelFormat::Undefined;
    _detach = nullptr;
    _attach = nullptr;
    _engine = nullptr;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
