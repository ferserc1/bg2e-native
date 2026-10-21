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

#include <bg2e/bake/IntegratedBakerContext.hpp>
#include <bg2e/bake/LightmapCompositionPass.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::bake {
namespace {
void advance(uint64_t& generation)
{
    if (generation == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Bake generation exhausted");
    }
    ++generation;
}

bool sameQueue(const draw::RenderQueue& a, const draw::RenderQueue& b)
{
    // These ABI structs consist entirely of initialized 16-byte vector lanes,
    // with static size/offset assertions in ShaderData.hpp (no padding bytes).
    if (a.items().size() != b.items().size() ||
        std::memcmp(&a.lights(),&b.lights(),sizeof(draw::LightUniforms)) != 0)
    {
        return false;
    }
    const auto ae = a.environment(), be = b.environment();
    if (bool(ae) != bool(be) || (ae && (ae->environmentImage() != be->environmentImage() ||
        ae->skyboxBlur() != be->skyboxBlur() || ae->skyboxVisible() != be->skyboxVisible())))
    {
        return false;
    }
    for (size_t i = 0; i < a.items().size(); ++i)
    {
        const auto& x = a.items()[i];
        const auto& y = b.items()[i];
        if (x.node.lock() != y.node.lock() || x.submeshIndex != y.submeshIndex ||
            x.drawMesh != y.drawMesh || x.drawMaterial != y.drawMaterial ||
            x.tracingMesh != y.tracingMesh || x.rayTracingEnabled != y.rayTracingEnabled ||
            x.textureImages != y.textureImages || x.textureSamplers != y.textureSamplers ||
            std::memcmp(&x.worldMatrix,&y.worldMatrix,sizeof(glm::mat4)) != 0 ||
            std::memcmp(&x.materialData,&y.materialData,sizeof(draw::MaterialData)) != 0)
        {
            return false;
        }
    }
    return true;
}
}

struct IntegratedBakeState {
    BakerContextConfiguration configuration;
    draw::Engine* engine = nullptr;
    uint64_t generation = 1, lastSerial = 0;
    bool hasSerial = false, closed = false, invalid = false;
    draw::RenderQueue baseline;
    std::unique_ptr<LightmapBakeExecutor> executor;
    std::shared_ptr<UvTemporalAccumulator> accumulator;
    std::shared_ptr<LightmapCompositionPass> composition;
    std::weak_ptr<IntegratedLightmapBaker> active;
    struct Prepared {
        gpu::CommandBuffer* command;
        uint32_t slot;
        uint64_t serial, generation;
        std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot;
        std::shared_ptr<BakeSample> sample;
        std::shared_ptr<const AccumulationResult> accumulation;
        std::optional<CompositionResult> composition;
        bool attempted = false, failed = false;
    };
    std::optional<Prepared> prepared;

    draw::RenderQueue extract() const
    {
        if (closed || !engine || engine->device() != configuration.device ||
            engine->backendType() != configuration.backend)
        {
            throw std::logic_error("Integrated bake context/Engine is closed or replaced");
        }
        draw::RenderQueue queue;
        draw::RenderQueueVisitor visitor;
        visitor.enqueue(configuration.sceneRoot.get(),*engine,queue);
        return queue;
    }

    void checkSources()
    {
        if (invalid || !sameQueue(baseline,extract()))
        {
            if (!invalid)
            {
                advance(generation);
                invalid = true;
            }
            throw std::logic_error("Bake scene changed; retire/discard frame, invalidate sources and reset target");
        }
    }

    void cleanupOutput()
    {
        if (accumulator)
        {
            accumulator->cleanup();
        }
        if (composition)
        {
            composition->cleanup();
        }
        accumulator.reset();
        composition.reset();
    }

    void requireStopped() const
    {
        if (closed || prepared)
        {
            throw std::logic_error("Bake operation requires a live context with no open frame");
        }
    }

    Prepared& requireFrame(uint32_t slot, uint64_t serial)
    {
        if (closed || !prepared || prepared->slot != slot || prepared->serial != serial)
        {
            throw std::logic_error("Bake frame does not match the prepared slot/serial");
        }
        return *prepared;
    }
};

struct IntegratedLightmapBaker::Impl {
    std::shared_ptr<IntegratedBakeState> state;
    LightmapBakerConfiguration configuration;
    LightmapSettings frozen;
    BakeProgress progress;
    std::shared_ptr<const UvSurfaceGeometry> geometry;
    glm::mat4 nodeWorld{1};
    std::shared_ptr<const BakeSample> latest;
    std::shared_ptr<const AccumulationResult> accumulated;
    std::optional<LightmapResult> output;
    std::optional<LightmapPixels> pixels;
    bool cleaned = false;

    void checkTarget() const
    {
        auto node = configuration.target;
        bool found = false;
        for (auto* ancestor = node.get(); ancestor; ancestor = ancestor->parent())
        {
            if (ancestor->disabled())
            {
                throw std::logic_error("Bake target has a disabled ancestor");
            }
            found = found || ancestor == state->configuration.sceneRoot.get();
        }
        auto* component = node->drawable();
        auto drawable = component ? component->drawable() : nullptr;
        const auto world = node->worldMatrix();
        if (!found || !drawable || !geometry || drawable != geometry->target ||
            drawable->drawMesh() != geometry->drawMesh ||
            std::memcmp(&world,&nodeWorld,sizeof(glm::mat4)) != 0)
        {
            throw std::logic_error("Bake target was removed, replaced, reloaded or transformed; invalidate and reset");
        }
    }
};

IntegratedBakerContext::IntegratedBakerContext(draw::Engine& engine, BakerContextConfiguration configuration)
    : _state(std::make_shared<IntegratedBakeState>())
{
    configuration.validate();
    if (configuration.environment)
    {
        configuration.environment->validate(configuration.device);
        configuration.environment = std::make_shared<draw::environment::EnvironmentResources>(*configuration.environment);
    }
    _state->configuration = std::move(configuration);
    _state->engine = &engine;
    _state->baseline = _state->extract();
    _state->executor = std::make_unique<LightmapBakeExecutor>(_state->configuration);
}

IntegratedBakerContext::~IntegratedBakerContext()
{
    if (_state->closed)
    {
        return;
    }
    try
    {
        // Lifetime precondition: unsubmitted commands have already been discarded.
        // Drain submitted work even if the caller omitted completion notification.
        _state->configuration.device->waitIdle();
        _state->prepared.reset();
        cleanup();
    }
    catch (...)
    {
        _state->closed = true;
    }
}

const BakerContextConfiguration& IntegratedBakerContext::configuration() const
{
    return _state->configuration;
}

uint64_t IntegratedBakerContext::sourceGeneration() const
{
    return _state->generation;
}

std::shared_ptr<IntegratedLightmapBaker> IntegratedBakerContext::createBaker(LightmapBakerConfiguration configuration)
{
    _state->requireStopped();
    if (!_state->active.expired())
    {
        throw std::logic_error("Integrated context supports one live target; release it before creating another");
    }
    configuration.validate();
    auto baker = std::shared_ptr<IntegratedLightmapBaker>(new IntegratedLightmapBaker(_state,std::move(configuration)));
    _state->active = baker;
    baker->resetAccumulation();
    return baker;
}

void IntegratedBakerContext::prepareFrame(const BakeFrame& frame)
{
    _state->requireStopped();
    auto baker = _state->active.lock();
    if (!baker || baker->_impl->cleaned || frame.command.hasActiveScope() ||
        frame.slot >= configuration().frameSlots || frame.sourceGeneration != sourceGeneration() ||
        (_state->hasSerial && frame.serial <= _state->lastSerial))
    {
        throw std::logic_error("Bake preparation requires current target/generation, completed slot and increasing serial");
    }
    _state->checkSources();
    baker->_impl->checkTarget();
    const auto progress = baker->progress();
    if ((progress.state != BakeState::Ready && progress.state != BakeState::Running) ||
        progress.completedSamples >= progress.requestedSamples)
    {
        throw std::logic_error("Bake target does not accept another sample; reset or compose completed samples");
    }
    // Consume serial even if preparation fails: partially replaced slots are not retryable.
    _state->hasSerial = true;
    _state->lastSerial = frame.serial;
    try
    {
        auto snapshot = _state->executor->prepareFrame(frame.slot,frame.serial,_state->baseline);
        _state->prepared = IntegratedBakeState::Prepared{&frame.command,frame.slot,frame.serial,
            frame.sourceGeneration,std::move(snapshot),nullptr,nullptr,std::nullopt,false,false};
    }
    catch (...)
    {
        baker->_impl->progress.state = BakeState::Failed;
        baker->_impl->progress.status = "Scene/profile preparation failed; reset target before retry";
        throw;
    }
}

void IntegratedBakerContext::prepareFrame(const draw::FrameContext& frame)
{
    if (&frame.engine != _state->engine)
    {
        throw std::invalid_argument("Presentation frame belongs to another draw Engine");
    }
    prepareFrame({frame.commandBuffer,frame.frameSlot,frame.frameNumber,sourceGeneration()});
}

void IntegratedBakerContext::completeFrame(uint32_t slot, uint64_t serial)
{
    auto& prepared = _state->requireFrame(slot,serial);
    auto baker = _state->active.lock();
    if (!prepared.failed && prepared.sample && baker && !baker->_impl->cleaned)
    {
        try
        {
            _state->checkSources();
            baker->_impl->checkTarget();
        }
        catch (...)
        {
            _state->invalid = true;
            _state->prepared.reset(); // GPU completion was confirmed; publish no stale sample.
            throw;
        }
    }
    if (!prepared.failed && prepared.sample && baker && !baker->_impl->cleaned &&
        prepared.generation == sourceGeneration() && !_state->invalid &&
        baker->_impl->progress.state != BakeState::Cancelled &&
        baker->_impl->progress.targetGeneration == prepared.sample->surface.targetGeneration)
    {
        auto& target = *baker->_impl;
        auto& progress = target.progress;
        try
        {
            if (!prepared.accumulation || prepared.accumulation->updates != progress.completedSamples+1)
            {
                throw std::logic_error("Missing or out-of-sequence GPU accumulation");
            }
            if (prepared.composition)
            {
                if (prepared.composition->sourceGeneration != progress.sourceGeneration ||
                    prepared.composition->targetGeneration != progress.targetGeneration ||
                    prepared.composition->mode != target.frozen.mode())
                {
                    throw std::logic_error("Composition replacement returned mismatched job metadata");
                }
                UvTemporalAccumulator::validateCompleted(*prepared.accumulation,progress.requestedSamples);
                auto pixels = LightmapCompositionPass::readPixels(*prepared.composition,target.frozen);
                target.pixels = std::move(pixels);
                target.output = LightmapResult{prepared.composition->image,
                    {target.frozen.resolution(),target.frozen.resolution()},target.frozen.mode(),
                    LightmapSignal::IndirectMultiplierRGB,1,progress.sourceGeneration,progress.targetGeneration};
            }
            target.accumulated = prepared.accumulation;
            target.latest = prepared.sample;
            ++progress.completedSamples;
            progress.state = prepared.composition ? BakeState::Complete : BakeState::Running;
            progress.status = prepared.composition ? "Completed and validated lightmap" : "Raw sample accumulated";
        }
        catch (...)
        {
            target.output.reset();
            target.pixels.reset();
            progress.state = BakeState::Failed;
            progress.status = "Lightmap validation/readback failed; no output published";
            _state->prepared.reset(); // Confirmed completed submission; retained owners remain safe.
            throw;
        }
    }
    _state->prepared.reset(); // Executor retains submitted owners until completed-slot reuse/cleanup.
}

void IntegratedBakerContext::abandonFrame(uint32_t slot, uint64_t serial)
{
    _state->requireFrame(slot,serial);
    _state->prepared.reset(); // Caller has discarded the unsubmitted command buffer.
}

void IntegratedBakerContext::setEnvironmentResources(
    std::shared_ptr<const draw::environment::EnvironmentResources> environment)
{
    _state->requireStopped();
    if (environment)
    {
        environment->validate(configuration().device);
        environment = std::make_shared<draw::environment::EnvironmentResources>(*environment);
    }
    auto next = _state->configuration;
    next.environment = std::move(environment);
    auto executor = std::make_unique<LightmapBakeExecutor>(next);
    invalidateSources();
    _state->executor->cleanup();
    _state->configuration = std::move(next);
    _state->executor = std::move(executor);
}

void IntegratedBakerContext::invalidateSources()
{
    _state->requireStopped();
    configuration().device->waitIdle();
    advance(_state->generation);
    _state->invalid = true;
    auto next = _state->extract();
    _state->baseline = std::move(next);
    // Only resetAccumulation establishes a new target job against these sources.
}

void IntegratedBakerContext::cleanup()
{
    if (_state->closed)
    {
        return;
    }
    _state->requireStopped();
    configuration().device->waitIdle();
    _state->executor->cleanup();
    _state->cleanupOutput();
    _state->closed = true;
    _state->baseline.clear();
}

IntegratedLightmapBaker::IntegratedLightmapBaker(std::shared_ptr<IntegratedBakeState> state,
    LightmapBakerConfiguration configuration) : _impl(std::make_unique<Impl>())
{
    _impl->state = std::move(state);
    _impl->configuration = std::move(configuration);
}

IntegratedLightmapBaker::~IntegratedLightmapBaker()
{
    // Context owns all in-flight owners independently of this target handle.
    cancel();
}

const LightmapBakerConfiguration& IntegratedLightmapBaker::configuration() const
{
    return _impl->configuration;
}

BakeProgress IntegratedLightmapBaker::progress() const
{
    auto progress = _impl->progress;
    if (_impl->state->closed || _impl->state->invalid ||
        progress.sourceGeneration != _impl->state->generation)
    {
        progress.state = BakeState::Invalidated;
        progress.status = "Sources/context invalidated; retire outstanding work and reset";
    }
    return progress;
}

const LightmapSettings& IntegratedLightmapBaker::frozenSettings() const
{
    return _impl->frozen;
}

std::shared_ptr<const BakeSample> IntegratedLightmapBaker::latestSample() const
{
    const auto state = progress().state;
    return state == BakeState::Running || state == BakeState::Complete ? _impl->latest : nullptr;
}

void IntegratedLightmapBaker::resetAccumulation()
{
    auto& state = *_impl->state;
    state.requireStopped();
    configuration().validate();
    auto queue = state.extract();
    // An explicit invalidation captures the new scene. Reset alone cannot bless edits.
    if (!sameQueue(state.baseline,queue))
    {
        throw std::logic_error("Notify invalidateSources before restarting after scene edits");
    }
    auto* component = configuration().target->drawable();
    auto drawable = component ? component->drawable() : nullptr;
    if (!drawable)
    {
        throw std::invalid_argument("Integrated bake target requires a loaded draw Drawable");
    }
    state.configuration.device->waitIdle();
    advance(_impl->progress.targetGeneration);
    _impl->latest.reset();
    _impl->accumulated.reset();
    _impl->output.reset();
    _impl->pixels.reset();
    _impl->geometry.reset();
    _impl->progress.state = BakeState::Failed;
    _impl->progress.status = "Target initialization failed";
    _impl->cleaned = false;
    state.executor->cleanup();
    state.cleanupOutput();
    _impl->frozen = *configuration().settings;
    state.executor->initialize(_impl->frozen);
    _impl->geometry = state.executor->prepareGeometry(drawable);
    _impl->nodeWorld = configuration().target->worldMatrix();
    _impl->checkTarget();
    AccumulationConfiguration accumulation;
    accumulation.shaderRoot = state.configuration.shaderRoot;
    CompositionConfiguration composition;
    composition.shaderRoot = state.configuration.shaderRoot;
    state.accumulator = state.configuration.passes.accumulator ? state.configuration.passes.accumulator :
        std::make_shared<UvTemporalAccumulator>(accumulation);
    state.composition = state.configuration.passes.composition ? state.configuration.passes.composition :
        std::make_shared<LightmapCompositionPass>(composition);
    state.accumulator->initialize(*state.configuration.device,state.configuration.backend,state.configuration.frameSlots);
    state.composition->initialize(*state.configuration.device,state.configuration.backend,state.configuration.frameSlots);
    state.invalid = false;
    _impl->progress = {BakeState::Ready,0,_impl->frozen.accumulationFrames(),state.generation,
        _impl->progress.targetGeneration,"Ready for raw UV-space AO/GI samples"};
}

void IntegratedLightmapBaker::cancel()
{
    _impl->progress.state = BakeState::Cancelled;
    _impl->progress.status = "Cancelled; submitted owners retained until frame retirement";
    _impl->latest.reset();
    _impl->output.reset();
    _impl->pixels.reset();
}

void IntegratedLightmapBaker::cleanup()
{
    auto& state = *_impl->state;
    if (!state.closed)
    {
        state.requireStopped();
        state.executor->cleanup();
        state.cleanupOutput();
    }
    cancel();
    _impl->accumulated.reset();
    _impl->geometry.reset();
    _impl->cleaned = true;
}

void IntegratedLightmapBaker::record(const BakeFrame& frame)
{
    auto& state = *_impl->state;
    auto& prepared = state.requireFrame(frame.slot,frame.serial);
    if (_impl->cleaned || prepared.attempted || prepared.command != &frame.command ||
        frame.command.hasActiveScope() || frame.sourceGeneration != state.generation ||
        prepared.generation != state.generation || progress().state == BakeState::Cancelled ||
        (progress().state != BakeState::Ready && progress().state != BakeState::Running))
    {
        throw std::logic_error("Bake update requires the exact prepared command/slot/serial/generations once");
    }
    prepared.attempted = true;
    try
    {
        state.checkSources();
        _impl->checkTarget();
        prepared.sample = std::make_shared<BakeSample>(state.executor->record(
            {frame.command,frame.slot,frame.serial,state.generation,_impl->progress.targetGeneration,
                _impl->geometry,_impl->nodeWorld,{_impl->frozen.resolution(),_impl->frozen.resolution()}},
            _impl->progress.completedSamples,prepared.snapshot,state.baseline.lights()));
        prepared.accumulation = state.accumulator->record(frame.command,frame.slot,frame.serial,
            *prepared.sample,_impl->accumulated);
        if (!prepared.accumulation || prepared.accumulation->updates != _impl->progress.completedSamples+1 ||
            prepared.accumulation->mode != _impl->frozen.mode() ||
            prepared.accumulation->surface.sourceGeneration != state.generation ||
            prepared.accumulation->surface.targetGeneration != _impl->progress.targetGeneration ||
            prepared.accumulation->surface.geometry != _impl->geometry)
        {
            throw std::logic_error("Accumulation replacement returned mismatched job/geometry metadata");
        }
        if (prepared.accumulation->updates == _impl->frozen.accumulationFrames())
        {
            prepared.composition = state.composition->record(frame.command,frame.slot,frame.serial,
                *prepared.accumulation,_impl->frozen,prepared.sample->environment);
        }
        _impl->progress.state = BakeState::Running;
        _impl->progress.status = "Sample/accumulation recorded; awaiting explicit GPU completion";
    }
    catch (...)
    {
        prepared.failed = true;
        _impl->progress.state = BakeState::Failed;
        _impl->progress.status = "Recording failed; discard unsubmitted command and reset target";
        throw;
    }
}

std::optional<LightmapResult> IntegratedLightmapBaker::result() const
{
    if (_impl->cleaned || progress().state != BakeState::Complete || !_impl->output)
    {
        return std::nullopt;
    }
    try
    {
        _impl->state->checkSources();
        _impl->checkTarget();
    }
    catch (...)
    {
        _impl->state->invalid = true;
        return std::nullopt;
    }
    return _impl->output;
}

LightmapPixels IntegratedLightmapBaker::readPixels() const
{
    if (!result() || !_impl->pixels)
    {
        throw std::logic_error("Current completed and validated lightmap pixels are unavailable");
    }
    return *_impl->pixels;
}
}
