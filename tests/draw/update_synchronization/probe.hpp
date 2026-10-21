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

#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {
// Initialized delegate with settled environment/settings, empty pending queue,
// standard deferred/final/temporal/upscaler types and stopped CPU producers.
// This source probe drives prepareFrame and may invoke lifecycle/native work.
inline bool runUpdateSynchronizationProbe(DefaultRenderLoopDelegate& delegate)
{
    auto* renderer = dynamic_cast<RendererDeferred*>(delegate.renderer());
    if (!renderer || renderer->configuration().layers.size() <= 2)
    {
        return false;
    }
    auto final = std::dynamic_pointer_cast<pipeline::FinalLayer>(renderer->configuration().layers[2].layer);
    if (!final)
    {
        return false;
    }
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    const auto generation = delegate.rendererGeneration();
    const float original = final->exposure();
    const float first = original == 1.0f ? 2.0f : 1.0f;
    std::vector<ValueUpdateResult> results;
    const auto notify = [&](ValueUpdateResult result)
    {
        results.push_back(result);
    };
    if (!delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, first, notify))
    {
        return false;
    }
    const auto history = renderer->historyGeneration();
    delegate.prepareFrame();
    require(results.size() == 1 && results[0] == ValueUpdateResult::Applied && final->exposure() == first,
        "Typed values must apply before notification");
    require(renderer->historyGeneration() == history+1, "Changed final values must retain logical invalidation");
    results.clear();
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, first, notify);
    delegate.prepareFrame();
    require(results.size() == 1 && results[0] == ValueUpdateResult::Unchanged &&
        renderer->historyGeneration() == history+1, "Identical assignments must avoid repeated invalidation");
    results.clear();
    delegate.enqueueFinalColorUpdate(generation+1, FinalColorProperty::Exposure, 3, notify);
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 3, notify, []
    {
        return false;
    });
    delegate.prepareFrame();
    require(results.size() == 2 && results[0] == ValueUpdateResult::Stale && results[1] == ValueUpdateResult::Stale &&
        final->exposure() == first, "Generation and catalog guards must reject stale values");
    // No observers: only the last adjacent assignment needs publishing.
    const auto beforeCoalescing = renderer->historyGeneration();
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 4);
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 5);
    delegate.prepareFrame();
    require(final->exposure() == 5 && renderer->historyGeneration() == beforeCoalescing+1,
        "Adjacent unobserved writes must publish/invalidate only once");
    // A generic callback is a coalescing/order barrier and retains the drain.
    int observations = 0;
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 6);
    delegate.enqueueRendererUpdate(generation, [&](Renderer&)
    {
        require(final->exposure() == 6, "Generic callback must observe preceding typed writes");
        ++observations;
    });
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 7);
    delegate.prepareFrame();
    require(observations == 1 && final->exposure() == 7, "Mixed batches must preserve order");
    // Notification work queues into the next prepare snapshot.
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 8, [&](ValueUpdateResult)
    {
        delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, 9);
    });
    delegate.prepareFrame();
    require(final->exposure() == 8, "Reentrant queues must not execute in the current snapshot");
    delegate.prepareFrame();
    require(final->exposure() == 9, "Reentrant queues must execute on the next prepare");
    bool rejected = false;
    try
    {
        delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Gamma, std::numeric_limits<float>::quiet_NaN());
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "Invalid values must fail before enqueue");
    delegate.enqueueFinalColorUpdate(generation, FinalColorProperty::Exposure, original);
    delegate.prepareFrame();
    return true;
}
}
