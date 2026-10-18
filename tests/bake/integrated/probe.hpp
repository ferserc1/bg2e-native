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

#include <bg2e/bake/all.hpp>
#include <bg2e/draw/Engine.hpp>
#include <stdexcept>

namespace bg2e::bake::tests {
// Stopped, initialized draw harness with loaded UV2 target in root. For GI the
// traceable scene must be nonempty and retain matching explicit environment.
// Required unsupported profiles throw rather than accepting a fallback bake.
inline void runIntegratedBakeProbe(draw::Engine& engine, std::shared_ptr<scene::Node> root,
    std::shared_ptr<scene::Node> target,
    std::shared_ptr<const draw::environment::EnvironmentResources> environment,
    const std::filesystem::path& shaderRoot, LightmapMode mode)
{
    auto require = [](bool condition, const char* message)
    {
        if (!condition)
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
        require(failed,"Invalid integrated bake lifecycle unexpectedly accepted");
    };
    BakerContextConfiguration configuration;
    configuration.device = engine.device();
    configuration.backend = engine.backendType();
    configuration.frameSlots = 2;
    configuration.shaderRoot = shaderRoot;
    configuration.sceneRoot = root;
    configuration.environment = environment;
    IntegratedBakerContext context(engine,configuration);
    LightmapBakerConfiguration targetConfiguration;
    targetConfiguration.target = target;
    auto settings = targetConfiguration.settings;
    settings->setMode(mode);
    settings->setResolution(33);
    settings->setDilationPixels(2);
    settings->setAccumulationFrames(2);
    settings->setSamplesPerPixel(2);
    auto baker = context.createBaker(targetConfiguration);
    reject([&]
    {
        context.createBaker(targetConfiguration);
    });
    const auto generation = context.sourceGeneration();
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* commandPointer)
    {
        auto& command = *commandPointer;
        BakeFrame frame{command,0,10,generation};
        reject([&]
        {
            baker->record(frame); // Not prepared.
        });
        reject([&]
        {
            context.prepareFrame({command,2,10,generation}); // Outside acquired slots.
        });
        reject([&]
        {
            context.prepareFrame({command,0,10,generation+1});
        });
        context.prepareFrame(frame);
        reject([&]
        {
            context.prepareFrame(frame); // One open bake frame.
        });
        reject([&]
        {
            baker->record({command,0,11,generation});
        });
        baker->record(frame);
        reject([&]
        {
            baker->record(frame); // Duplicate sample in prepared serial.
        });
        require(baker->progress().completedSamples == 0 && !baker->latestSample(),
            "Recording must not count or publish an unfinished GPU sample");
    });
    context.completeFrame(0,10); // immediateSubmit returned after completion.
    auto first = baker->latestSample();
    require(first && first->signal && first->snapshot && first->sampleIndex == 0 &&
        first->surface.sourceGeneration == generation && first->mode == mode &&
        first->signal->size() == gpu::Size2D{33,33} && baker->progress().completedSamples == 1,
        "Completed raw sample ownership/metadata/progress mismatch");
    require(!baker->result(),"Raw samples must not publish a composed lightmap");
    reject([&]
    {
        baker->readPixels();
    });
    reject([&]
    {
        context.completeFrame(0,10);
    });
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* commandPointer)
    {
        auto& command = *commandPointer;
        reject([&]
        {
            context.prepareFrame({command,1,10,generation}); // Reused global serial, different slot.
        });
        BakeFrame frame{command,1,11,generation};
        context.prepareFrame(frame);
        baker->record(frame);
        baker->cancel();
        require(!baker->latestSample(),"Cancelled target must hide its old sample");
        reject([&]
        {
            baker->resetAccumulation(); // Pending submitted/recorded owner cannot be dropped.
        });
        reject([&]
        {
            context.invalidateSources();
        });
    });
    context.completeFrame(1,11);
    require(baker->progress().state == BakeState::Cancelled && baker->progress().completedSamples == 1,
        "Cancelled pending sample must not advance completion");
    const auto targetGeneration = baker->progress().targetGeneration;
    context.invalidateSources();
    require(context.sourceGeneration() > generation && baker->progress().state == BakeState::Invalidated,
        "Source invalidation must invalidate the old target job");
    baker->resetAccumulation();
    require(baker->progress().completedSamples == 0 && baker->progress().targetGeneration > targetGeneration &&
        baker->progress().sourceGeneration == context.sourceGeneration() && !baker->latestSample(),
        "Reset must capture fresh generations and clear completed samples");
    for (uint32_t sample = 0; sample < 2; ++sample)
    {
        engine.device()->immediateSubmit([&](gpu::CommandBuffer* commandPointer)
        {
            auto& command = *commandPointer;
            BakeFrame frame{command,sample,12+sample,context.sourceGeneration()};
            context.prepareFrame(frame);
            baker->record(frame);
        });
        context.completeFrame(sample,12+sample);
        require(baker->progress().completedSamples == sample+1 &&
            baker->latestSample()->sampleIndex == sample,"Reset sample sequence must restart independently of slots");
    }
    require(baker->progress().state == BakeState::Complete && baker->result().has_value(),
        "Final completed/validated composition must publish a current lightmap");
    auto pixels = baker->readPixels();
    pixels.validate();
    require(pixels.width == 33 && pixels.height == 33 && baker->result()->uvSet == 1,
        "Completed CPU/GPU lightmap must retain frozen extent and UV2 metadata");
    require(first->signal && first->surface.images[0],"Retained old sample must survive slot reuse/reset");
    baker->cleanup();
    context.cleanup();
}
}
