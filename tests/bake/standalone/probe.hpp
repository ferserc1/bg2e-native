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

#include <bg2e/bake/StandaloneBakerContext.hpp>
#include <bg2e/bake/UvTemporalAccumulator.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/scene/Node.hpp>
#include <functional>
#include <stdexcept>
#include <utility>

namespace bg2e::bake::tests {
// Call with a stopped initialized offscreen Engine, loaded camera-less root,
// two enabled UV2 targets and optional explicit environment. No target/autobuild.
// Unsupported required profiles must throw; test those separately from this probe.
inline void runStandaloneBakeProbe(draw::Engine& engine, std::shared_ptr<scene::Node> root,
    std::shared_ptr<scene::Node> firstTarget, std::shared_ptr<scene::Node> secondTarget,
    std::shared_ptr<const draw::environment::EnvironmentResources> environment,
    const std::filesystem::path& shaderRoot, LightmapMode mode)
{
    auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    auto reject = [&](const auto& operation)
    {
        bool failed = false;
        try
        {
            operation();
        }
        catch (const std::exception&)
        {
            failed = true;
        }
        require(failed,"Invalid standalone operation was accepted");
    };
    // Inject failure/cancellation after UV work has been recorded, exercising
    // abandoned-command and completed-but-cancelled publication separately.
    class ControlledAccumulator final : public UvTemporalAccumulator {
      public:
        explicit ControlledAccumulator(AccumulationConfiguration configuration)
            : UvTemporalAccumulator(std::move(configuration))
        {
        }

        std::function<void()> beforeRecord;

        std::shared_ptr<const AccumulationResult> record(gpu::CommandBuffer& command,
            uint32_t slot, uint64_t serial, const BakeSample& sample,
            std::shared_ptr<const AccumulationResult> previous) override
        {
            if (beforeRecord)
            {
                beforeRecord();
            }
            return UvTemporalAccumulator::record(command,slot,serial,sample,std::move(previous));
        }
    };
    auto accumulator = std::make_shared<ControlledAccumulator>(AccumulationConfiguration{shaderRoot});
    BakePassConfiguration passes;
    passes.accumulator = accumulator;
    StandaloneBakerContext context(engine);
    require(context.state() == StandaloneBakerContext::State::Created,"Missing Created state");
    reject([&]
    {
        context.prepareScene(root,environment);
    });
    context.initialize(shaderRoot,passes);
    require(context.state() == StandaloneBakerContext::State::Initialized,"Missing Initialized state");
    reject([&]
    {
        context.initialize(shaderRoot);
    });
    LightmapBakerConfiguration target;
    target.target = firstTarget;
    target.settings->setMode(mode);
    target.settings->setResolution(33);
    target.settings->setAccumulationFrames(2);
    target.settings->setSamplesPerPixel(2);
    target.settings->setDilationPixels(2);
    target.settings->setCpuFormat(LightmapPixelFormat::RGB32F);
    reject([&]
    {
        context.createBaker(target);
    });
    context.prepareScene(root,environment);
    const auto generation = context.sourceGeneration();
    auto baker = context.createBaker(target);
    reject([&]
    {
        context.createBaker(target);
    });
    reject([&]
    {
        baker->readPixels();
    });
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        reject([&]
        {
            baker->record({*command,0,1,generation});
        });
    });
    baker->update();
    auto firstSample = baker->latestSample();
    require(firstSample && firstSample->sampleIndex == 0 && baker->progress().completedSamples == 1 &&
        !baker->result(),"First completed sample must not publish a final map");
    target.settings->setAccumulationFrames(3);
    require(baker->frozenSettings().accumulationFrames() == 2,"Live settings changed the running job");
    baker->update();
    auto output = baker->result();
    require(output && output->uvSet == 1 && output->sourceGeneration == generation &&
        baker->progress().state == BakeState::Complete && baker->progress().completedSamples == 2,
        "Standalone final output/count metadata mismatch");
    baker->readPixels().validate();
    reject([&]
    {
        baker->update();
    });
    const auto targetGeneration = baker->progress().targetGeneration;
    baker->resetAccumulation();
    require(baker->frozenSettings().accumulationFrames() == 3 &&
        baker->progress().targetGeneration > targetGeneration,"Reset did not freeze fresh settings/generation");
    accumulator->beforeRecord = [&]
    {
        reject([&]
        {
            baker->update();
        });
        reject([&]
        {
            context.invalidateSources();
        });
        baker->cancel();
    };
    baker->update();
    require(baker->progress().state == BakeState::Cancelled && baker->progress().completedSamples == 0 &&
        !baker->result() && !baker->latestSample(),"Cancelled completed work was published");
    accumulator->beforeRecord = {};
    baker->resetAccumulation();
    accumulator->beforeRecord = []
    {
        throw std::runtime_error("Injected partial standalone recording failure");
    };
    reject([&]
    {
        baker->update();
    });
    require(baker->progress().state == BakeState::Failed && baker->progress().completedSamples == 0 &&
        !baker->result(),"Failed command advanced standalone output");
    accumulator->beforeRecord = {};
    baker->resetAccumulation();
    baker->update();
    context.prepareScene(root,environment);
    require(context.sourceGeneration() > generation && baker->progress().state == BakeState::Invalidated,
        "New scene did not invalidate the old job");
    reject([&]
    {
        baker->update();
    });
    reject([&]
    {
        baker->resetAccumulation(); // A stale job cannot revive against a new scene.
    });
    baker.reset();
    target.target = secondTarget;
    target.settings->setAccumulationFrames(1);
    auto second = context.createBaker(target);
    second->update();
    require(second->result().has_value() && second->progress().completedSamples == 1,
        "Second target failed in the same standalone context");
    auto secondOutput = second->result();
    root->setEnabled(false);
    require(!second->result() && context.state() == StandaloneBakerContext::State::Initialized,
        "Unannounced scene edits did not invalidate completed output");
    root->setEnabled(true);
    reject([&]
    {
        second->resetAccumulation();
    });
    context.cleanup();
    context.cleanup();
    second->cleanup();
    require(context.state() == StandaloneBakerContext::State::Cleaned && !second->result() &&
        output->image && firstSample->signal && secondOutput->image,
        "Cleanup or independently retained output lifetime failed");
    reject([&]
    {
        context.prepareScene(root,environment);
    });
}
}
