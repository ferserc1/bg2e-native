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

#include <bg2e/bake/StandaloneBakerContext.hpp>
#include <bg2e/bake/LightmapCompositionPass.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::bake {
namespace {
void advance(uint64_t& value)
{
    if (value == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Standalone bake generation/serial exhausted");
    }
    ++value;
}

bool sameColor(const base::Color& a, const base::Color& b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

bool sameEnvironment(const std::shared_ptr<const scene::EnvironmentComponent>& a,
    const std::shared_ptr<const scene::EnvironmentComponent>& b)
{
    return bool(a) == bool(b) && (!a || (a->environmentImage() == b->environmentImage() &&
        a->skyboxBlur() == b->skyboxBlur() && a->skyboxVisible() == b->skyboxVisible() &&
        sameColor(a->skyHorizonColor(),b->skyHorizonColor()) &&
        sameColor(a->skyZenithColor(),b->skyZenithColor()) && sameColor(a->groundColor(),b->groundColor())));
}

bool sameQueue(const draw::RenderQueue& a, const draw::RenderQueue& b)
{
    if (a.items().size() != b.items().size() ||
        std::memcmp(&a.lights(),&b.lights(),sizeof(draw::LightUniforms)) != 0 ||
        !sameEnvironment(a.environment(),b.environment()))
    {
        return false;
    }
    for (size_t i = 0; i < a.items().size(); ++i)
    {
        const auto& x = a.items()[i];
        const auto& y = b.items()[i];
        if (x.node.lock() != y.node.lock() || x.nodeId != y.nodeId || x.objectId != y.objectId ||
            x.submeshIndex != y.submeshIndex || x.drawMesh != y.drawMesh || x.drawMaterial != y.drawMaterial ||
            x.tracingMesh != y.tracingMesh || x.rayTracingEnabled != y.rayTracingEnabled ||
            x.textureImages != y.textureImages || x.textureSamplers != y.textureSamplers ||
            x.textureRevisions != y.textureRevisions || x.imageGenerations != y.imageGenerations ||
            x.materialRevisions.values != y.materialRevisions.values ||
            x.materialRevisions.bindings != y.materialRevisions.bindings ||
            x.materialRevisions.classification != y.materialRevisions.classification ||
            std::memcmp(&x.worldMatrix,&y.worldMatrix,sizeof(glm::mat4)) != 0 ||
            std::memcmp(&x.materialData,&y.materialData,sizeof(draw::MaterialData)) != 0)
        {
            return false;
        }
    }
    return true;
}
}

// Shared validity/ownership survives the context handle, so late baker handles
// fail explicitly. Mutable execution resources remain exclusive to one target.
struct StandaloneBakeState {
    BakerContextConfiguration configuration;
    draw::Engine* engine = nullptr;
    StandaloneBakerContext::State lifecycle = StandaloneBakerContext::State::Created;
    uint64_t generation = 0, serial = 0, deviceIdentity = 0;
    bool busy = false, unsafe = false;
    draw::RenderQueue baseline;
    std::vector<uint64_t> blasGenerations;
    std::array<uint64_t,4> environmentGenerations{};
    std::unique_ptr<draw::environment::EnvironmentGenerator> generator;
    std::weak_ptr<StandaloneLightmapBaker> active;

    void requireEngine() const
    {
        if (!engine || !engine->device() || !engine->device()->isValid() || !engine->offscreenSurface() ||
            (configuration.device && (engine->device() != configuration.device ||
                engine->backendType() != configuration.backend || engine->device()->timingIdentity() != deviceIdentity)))
        {
            throw std::logic_error("Standalone baking requires the original initialized windowless draw Engine");
        }
    }

    void requireStopped() const
    {
        if (busy || unsafe || lifecycle == StandaloneBakerContext::State::Cleaned)
        {
            throw std::logic_error("Standalone bake operation requires a live stopped context and retired GPU work");
        }
        requireEngine();
    }

    draw::RenderQueue extract(scene::Node* root) const
    {
        requireEngine();
        draw::RenderQueue queue;
        draw::RenderQueueVisitor visitor;
        visitor.enqueue(root,*engine,queue);
        return queue;
    }

    std::array<uint64_t,4> environmentIdentity() const
    {
        const auto& e = configuration.environment;
        if (!e)
        {
            return {};
        }
        e->validate(configuration.device);
        return {e->sky->bindingGeneration(),e->irradiance->bindingGeneration(),
            e->prefilteredSpecular->bindingGeneration(),e->brdf->bindingGeneration()};
    }

    void checkSources()
    {
        if (lifecycle != StandaloneBakerContext::State::SceneReady)
        {
            throw std::logic_error("Prepare a standalone scene generation before baking");
        }
        try
        {
            if (!sameQueue(baseline,extract(configuration.sceneRoot.get())) ||
                environmentIdentity() != environmentGenerations)
            {
                throw std::logic_error("Standalone bake sources changed; prepare a new scene and baker");
            }
            for (size_t i = 0; i < baseline.items().size(); ++i)
            {
                const auto& mesh = baseline.items()[i].tracingMesh;
                if ((mesh ? mesh->buildGeneration() : 0) != blasGenerations[i])
                {
                    throw std::logic_error("Standalone bake BLAS changed; prepare a new scene and baker");
                }
            }
        }
        catch (...)
        {
            lifecycle = StandaloneBakerContext::State::Initialized;
            advance(generation);
            throw;
        }
    }
};

struct StandaloneLightmapBaker::Impl {
    std::shared_ptr<StandaloneBakeState> state;
    LightmapBakerConfiguration configuration;
    LightmapSettings frozen;
    BakeProgress progress;
    bool cleaned = false;
    std::unique_ptr<LightmapBakeExecutor> executor;
    std::shared_ptr<UvTemporalAccumulator> accumulator;
    std::shared_ptr<LightmapCompositionPass> composition;
    std::shared_ptr<const UvSurfaceGeometry> geometry;
    glm::mat4 nodeWorld{1};
    std::shared_ptr<const BakeSample> latest;
    std::shared_ptr<const AccumulationResult> accumulated;
    std::optional<LightmapResult> output;
    std::optional<LightmapPixels> pixels;
    struct Pending {
        gpu::CommandBuffer* command = nullptr;
        uint64_t serial = 0;
        bool attempted = false;
        std::shared_ptr<const gpu::detail::RecordingCompletion> completion;
        std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot;
        std::shared_ptr<BakeSample> sample;
        std::shared_ptr<const AccumulationResult> accumulation;
        std::optional<CompositionResult> composition;
    };
    std::optional<Pending> pending;

    void checkJob() const
    {
        if (cleaned || progress.sourceGeneration != state->generation ||
            state->lifecycle != StandaloneBakerContext::State::SceneReady)
        {
            throw std::logic_error("Standalone baker belongs to a retired scene generation or was cleaned");
        }
    }

    void checkTarget() const
    {
        auto node = configuration.target;
        bool found = false;
        for (auto* ancestor = node.get(); ancestor; ancestor = ancestor->parent())
        {
            if (ancestor->disabled())
            {
                throw std::logic_error("Standalone bake target has a disabled ancestor");
            }
            found = found || ancestor == state->configuration.sceneRoot.get();
        }
        auto* component = node->drawable();
        auto drawable = component ? component->drawable() : nullptr;
        const auto world = node->worldMatrix();
        if (!found || !drawable || !geometry || drawable != geometry->target || drawable->drawMesh() != geometry->drawMesh ||
            std::memcmp(&world,&nodeWorld,sizeof(glm::mat4)) != 0)
        {
            throw std::logic_error("Standalone bake target was removed, replaced, reloaded or transformed");
        }
    }

    void releaseExecution()
    {
        if (executor)
        {
            executor->cleanup();
        }
        if (accumulator)
        {
            accumulator->cleanup();
        }
        if (composition)
        {
            composition->cleanup();
        }
        pending.reset();
        executor.reset();
        accumulator.reset();
        composition.reset();
        geometry.reset();
        latest.reset();
        accumulated.reset();
        output.reset();
        pixels.reset();
    }
};

StandaloneBakerContext::StandaloneBakerContext(draw::Engine& engine)
    : _state(std::make_shared<StandaloneBakeState>())
{
    _state->engine = &engine;
}

StandaloneBakerContext::~StandaloneBakerContext()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
        _state->lifecycle = State::Cleaned;
    }
}

StandaloneBakerContext::State StandaloneBakerContext::state() const
{
    return _state->lifecycle;
}

const BakerContextConfiguration& StandaloneBakerContext::configuration() const
{
    return _state->configuration;
}

uint64_t StandaloneBakerContext::sourceGeneration() const
{
    return _state->generation;
}

void StandaloneBakerContext::initialize(std::filesystem::path shaderRoot, BakePassConfiguration passes)
{
    if (state() != State::Created)
    {
        throw std::logic_error("Initialize a standalone context exactly once");
    }
    _state->requireStopped();
    auto& configuration = _state->configuration;
    configuration.device = _state->engine->device();
    _state->deviceIdentity = configuration.device->timingIdentity();
    configuration.backend = _state->engine->backendType();
    configuration.frameSlots = 1;
    configuration.shaderRoot = shaderRoot.empty() ? base::PlatformTools::shaderPath() : std::move(shaderRoot);
    configuration.passes = std::move(passes);
    _state->lifecycle = State::Initialized;
}

void StandaloneBakerContext::prepareScene(std::shared_ptr<scene::Node> root,
    std::shared_ptr<const draw::environment::EnvironmentResources> environment)
{
    _state->requireStopped();
    if (state() == State::Created)
    {
        throw std::logic_error("Initialize standalone context before preparing a scene");
    }
    invalidateSources();
    base::CpuProfileScope profile("bake.standalone.prepareScene");
    if (!root)
    {
        throw std::invalid_argument("Standalone scene root must be retained");
    }
    auto configuration = _state->configuration;
    configuration.sceneRoot = std::move(root);
    configuration.validate();
    auto queue = _state->extract(configuration.sceneRoot.get());
    if (environment)
    {
        environment->validate(configuration.device);
    }
    else
    {
        if (!_state->generator)
        {
            _state->generator = std::make_unique<draw::environment::EnvironmentGenerator>(
                *_state->engine,configuration.shaderRoot);
        }
        _state->generator->resetSourceCache();
        _state->generator->synchronize(queue.environment().get());
        environment = _state->generator->resources();
    }
    configuration.environment = std::make_shared<draw::environment::EnvironmentResources>(*environment);
    std::vector<uint64_t> blasGenerations;
    for (const auto& item : queue.items())
    {
        blasGenerations.push_back(item.tracingMesh ? item.tracingMesh->buildGeneration() : 0);
    }
    _state->configuration = std::move(configuration);
    _state->baseline = std::move(queue);
    _state->blasGenerations = std::move(blasGenerations);
    _state->environmentGenerations = _state->environmentIdentity();
    _state->lifecycle = State::SceneReady;
}

std::shared_ptr<StandaloneLightmapBaker> StandaloneBakerContext::createBaker(LightmapBakerConfiguration configuration)
{
    _state->requireStopped();
    _state->checkSources();
    if (!_state->active.expired())
    {
        throw std::logic_error("Standalone context supports one live target; release it before the next");
    }
    configuration.validate();
    auto baker = std::shared_ptr<StandaloneLightmapBaker>(new StandaloneLightmapBaker(_state,std::move(configuration)));
    _state->active = baker;
    baker->resetAccumulation();
    return baker;
}

void StandaloneBakerContext::invalidateSources()
{
    _state->requireStopped();
    if (state() == State::Created)
    {
        throw std::logic_error("Initialize standalone context before invalidating sources");
    }
    advance(_state->generation);
    _state->lifecycle = State::Initialized;
    if (auto baker = _state->active.lock())
    {
        baker->cancel();
    }
    _state->configuration.device->waitIdle();
    _state->baseline.clear();
    _state->blasGenerations.clear();
}

void StandaloneBakerContext::cleanup()
{
    if (state() == State::Cleaned)
    {
        return;
    }
    if (_state->busy)
    {
        throw std::logic_error("Cannot clean standalone context during a sample");
    }
    if (state() != State::Created)
    {
        _state->requireEngine();
        _state->configuration.device->waitIdle();
        _state->unsafe = false;
        if (auto baker = _state->active.lock())
        {
            baker->cleanup();
        }
        if (_state->generator)
        {
            _state->generator->cleanup();
            _state->generator.reset();
        }
    }
    _state->lifecycle = State::Cleaned;
    _state->baseline.clear();
    _state->blasGenerations.clear();
    _state->configuration.sceneRoot.reset();
    _state->configuration.environment.reset();
    _state->configuration.passes = {};
}

StandaloneLightmapBaker::StandaloneLightmapBaker(std::shared_ptr<StandaloneBakeState> state,
    LightmapBakerConfiguration configuration) : _impl(std::make_unique<Impl>())
{
    _impl->state = std::move(state);
    _impl->configuration = std::move(configuration);
    _impl->progress.sourceGeneration = _impl->state->generation;
}

StandaloneLightmapBaker::~StandaloneLightmapBaker()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
        // Explicit cleanup reports device failures while the Engine is alive.
    }
}

const LightmapBakerConfiguration& StandaloneLightmapBaker::configuration() const
{
    return _impl->configuration;
}

BakeProgress StandaloneLightmapBaker::progress() const
{
    auto progress = _impl->progress;
    if (_impl->state->lifecycle != StandaloneBakerContext::State::SceneReady ||
        progress.sourceGeneration != _impl->state->generation)
    {
        progress.state = BakeState::Invalidated;
        progress.status = "Standalone scene generation retired; create a new baker";
    }
    return progress;
}

const LightmapSettings& StandaloneLightmapBaker::frozenSettings() const
{
    return _impl->frozen;
}

std::shared_ptr<const BakeSample> StandaloneLightmapBaker::latestSample() const
{
    if (progress().state != BakeState::Running && progress().state != BakeState::Complete)
    {
        return nullptr;
    }
    try
    {
        _impl->state->checkSources();
        _impl->checkTarget();
        return _impl->latest;
    }
    catch (...)
    {
        return nullptr;
    }
}

std::optional<LightmapResult> StandaloneLightmapBaker::result() const
{
    if (_impl->cleaned || progress().state != BakeState::Complete || !_impl->output)
    {
        return std::nullopt;
    }
    try
    {
        _impl->state->checkSources();
        _impl->checkTarget();
        return _impl->output;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

LightmapPixels StandaloneLightmapBaker::readPixels() const
{
    if (!result() || !_impl->pixels)
    {
        throw std::logic_error("Completed current standalone lightmap pixels are unavailable");
    }
    return *_impl->pixels;
}

void StandaloneLightmapBaker::resetAccumulation()
{
    auto& state = *_impl->state;
    state.requireStopped();
    // Retain this job through injected pass callbacks, which may release the
    // application's last handle while synchronous execution is still active.
    const auto owner = state.active.lock();
    if (owner.get() != this)
    {
        throw std::logic_error("Standalone operation requires the context's active target owner");
    }
    _impl->checkJob();
    state.checkSources();
    configuration().validate();
    auto* component = configuration().target->drawable();
    auto drawable = component ? component->drawable() : nullptr;
    if (!drawable || !drawable->isLoaded() || drawable->engine() != state.engine)
    {
        throw std::invalid_argument("Standalone target needs a loaded same-Engine draw Drawable");
    }
    base::CpuProfileScope profile("bake.standalone.reset");
    state.configuration.device->waitIdle();
    _impl->releaseExecution();
    advance(_impl->progress.targetGeneration);
    _impl->progress.state = BakeState::Failed;
    _impl->progress.completedSamples = 0;
    _impl->progress.status = "Standalone target initialization failed";
    _impl->frozen = *configuration().settings;
    state.busy = true;
    try
    {
        _impl->executor = std::make_unique<LightmapBakeExecutor>(state.configuration);
        _impl->executor->initialize(_impl->frozen);
        _impl->geometry = _impl->executor->prepareGeometry(drawable);
        _impl->nodeWorld = configuration().target->worldMatrix();
        _impl->checkTarget();
        AccumulationConfiguration accumulation;
        accumulation.shaderRoot = state.configuration.shaderRoot;
        CompositionConfiguration composition;
        composition.shaderRoot = state.configuration.shaderRoot;
        _impl->accumulator = state.configuration.passes.accumulator ? state.configuration.passes.accumulator :
            std::make_shared<UvTemporalAccumulator>(accumulation);
        _impl->composition = state.configuration.passes.composition ? state.configuration.passes.composition :
            std::make_shared<LightmapCompositionPass>(composition);
        _impl->accumulator->initialize(*state.configuration.device,state.configuration.backend,1);
        _impl->composition->initialize(*state.configuration.device,state.configuration.backend,1);
        // Preflight the actual effect-capacity snapshot and native acceleration.
        // No accumulation sample is counted; subsequent samples may reuse its
        // completed immutable acceleration through the existing public service.
        advance(state.serial);
        auto snapshot = _impl->executor->prepareFrame(0,state.serial,state.baseline);
        state.configuration.device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            snapshot->build(*command);
        });
        state.checkSources();
        _impl->checkTarget();
        if (_impl->progress.state == BakeState::Cancelled)
        {
            throw std::logic_error("Standalone target was cancelled during preparation");
        }
        _impl->progress = {BakeState::Ready,0,_impl->frozen.accumulationFrames(),state.generation,
            _impl->progress.targetGeneration,"Ready for synchronous standalone samples"};
        state.busy = false;
    }
    catch (...)
    {
        state.busy = false;
        state.unsafe = true;
        state.configuration.device->waitIdle();
        state.unsafe = false;
        _impl->releaseExecution();
        throw;
    }
}

void StandaloneLightmapBaker::cancel()
{
    _impl->progress.state = BakeState::Cancelled;
    _impl->progress.status = "Standalone target cancelled; no further sample publication";
    _impl->latest.reset();
    _impl->output.reset();
    _impl->pixels.reset();
}

void StandaloneLightmapBaker::cleanup()
{
    if (_impl->cleaned)
    {
        return;
    }
    auto& state = *_impl->state;
    if (state.busy)
    {
        throw std::logic_error("Cannot clean a standalone target during a sample");
    }
    if (_impl->executor || _impl->pending)
    {
        state.requireEngine();
        state.configuration.device->waitIdle();
        _impl->releaseExecution();
    }
    cancel();
    _impl->cleaned = true;
}

void StandaloneLightmapBaker::record(const BakeFrame& frame)
{
    auto& state = *_impl->state;
    auto& pending = _impl->pending;
    _impl->checkJob();
    if (!state.busy || !pending || pending->attempted || pending->command != &frame.command ||
        frame.slot != 0 || frame.serial != pending->serial || frame.sourceGeneration != state.generation ||
        frame.command.hasActiveScope() ||
        (progress().state != BakeState::Ready && progress().state != BakeState::Running))
    {
        throw std::logic_error("Standalone record accepts only the exact internal update command once");
    }
    pending->attempted = true;
    pending->sample = std::make_shared<BakeSample>(_impl->executor->record(
        {frame.command,0,frame.serial,state.generation,_impl->progress.targetGeneration,
            _impl->geometry,_impl->nodeWorld,{_impl->frozen.resolution(),_impl->frozen.resolution()}},
        _impl->progress.completedSamples,pending->snapshot,state.baseline.lights()));
    pending->accumulation = _impl->accumulator->record(frame.command,0,frame.serial,
        *pending->sample,_impl->accumulated);
    const auto& accumulation = pending->accumulation;
    if (!accumulation || accumulation->updates != _impl->progress.completedSamples+1 ||
        accumulation->mode != _impl->frozen.mode() ||
        accumulation->surface.sourceGeneration != state.generation ||
        accumulation->surface.targetGeneration != _impl->progress.targetGeneration ||
        accumulation->surface.geometry != _impl->geometry)
    {
        throw std::logic_error("Standalone accumulation returned mismatched job metadata");
    }
    if (accumulation->updates == _impl->frozen.accumulationFrames())
    {
        pending->composition = _impl->composition->record(frame.command,0,frame.serial,
            *accumulation,_impl->frozen,pending->sample->environment);
    }
}

void StandaloneLightmapBaker::update()
{
    auto& state = *_impl->state;
    state.requireStopped();
    // Retain this job through injected pass callbacks, which may release the
    // application's last handle while synchronous execution is still active.
    const auto owner = state.active.lock();
    if (owner.get() != this)
    {
        throw std::logic_error("Standalone operation requires the context's active target owner");
    }
    _impl->checkJob();
    state.checkSources();
    _impl->checkTarget();
    if ((progress().state != BakeState::Ready && progress().state != BakeState::Running) ||
        _impl->progress.completedSamples >= _impl->progress.requestedSamples)
    {
        throw std::logic_error("Standalone target does not accept another sample; reset or create a new job");
    }
    base::CpuProfileScope profile("bake.standalone.update");
    advance(state.serial);
    _impl->pending.emplace();
    _impl->pending->serial = state.serial;
    state.busy = true;
    try
    {
        auto& pending = *_impl->pending;
        pending.snapshot = _impl->executor->prepareFrame(0,state.serial,state.baseline);
        state.configuration.device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            pending.command = command;
            pending.completion = command->recordingCompletion();
            record({*command,0,state.serial,state.generation});
        });
        pending.command = nullptr;
        if (!pending.completion || !pending.completion->completed())
        {
            throw std::logic_error("Standalone sample has no successful completed recording");
        }
        state.checkSources();
        _impl->checkTarget();
        if (_impl->progress.state == BakeState::Cancelled)
        {
            _impl->pending.reset();
            state.busy = false;
            return;
        }
        if (pending.composition)
        {
            const auto& result = *pending.composition;
            if (!result.image || result.image->device() != state.configuration.device ||
                result.sourceGeneration != state.generation ||
                result.targetGeneration != _impl->progress.targetGeneration || result.mode != _impl->frozen.mode())
            {
                throw std::logic_error("Standalone composition returned mismatched job metadata");
            }
            UvTemporalAccumulator::validateCompleted(*pending.accumulation,_impl->progress.requestedSamples);
            auto pixels = LightmapCompositionPass::readPixels(result,_impl->frozen);
            _impl->pixels = std::move(pixels);
            _impl->output = LightmapResult{result.image,{_impl->frozen.resolution(),_impl->frozen.resolution()},
                result.mode,LightmapSignal::IndirectMultiplierRGB,1,state.generation,_impl->progress.targetGeneration};
        }
        _impl->latest = pending.sample;
        _impl->accumulated = pending.accumulation;
        ++_impl->progress.completedSamples;
        _impl->progress.state = pending.composition ? BakeState::Complete : BakeState::Running;
        _impl->progress.status = pending.composition ? "Completed and validated standalone lightmap" : "Completed standalone sample";
        base::CpuProfileScope::count("bake.standalone.completedSamples",1);
        _impl->pending.reset();
        state.busy = false;
    }
    catch (...)
    {
        _impl->progress.state = BakeState::Failed;
        _impl->progress.status = "Standalone sample failed; no output published";
        _impl->output.reset();
        _impl->pixels.reset();
        state.busy = false;
        // A failed submit/wait may still own native work. Keep Pending and all
        // primitive slot owners until a successful drain; never commit a sample.
        state.unsafe = true;
        state.configuration.device->waitIdle();
        state.unsafe = false;
        _impl->pending.reset();
        throw;
    }
}
}
