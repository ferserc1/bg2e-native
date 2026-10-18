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

#include <bg2e/bake/LightmapBaker.hpp>
#include <bg2e/bake/LightmapBakeExecutor.hpp>
#include <bg2e/draw/FrameContext.hpp>

namespace bg2e::bake {
struct IntegratedBakeState;
class IntegratedBakerContext;

class BG2E_API IntegratedLightmapBaker final : public LightmapBaker {
  public:
    ~IntegratedLightmapBaker() override;

    const LightmapBakerConfiguration& configuration() const override;
    BakeProgress progress() const override;
    const LightmapSettings& frozenSettings() const;
    std::shared_ptr<const BakeSample> latestSample() const; // Completed raw sample only.

    void resetAccumulation() override; // Stopped boundary; freezes live settings anew.
    void cancel() override;
    void cleanup() override;
    void record(const BakeFrame& frame) override;

    std::optional<LightmapResult> result() const override;
    LightmapPixels readPixels() const override;

  private:
    friend class IntegratedBakerContext;
    IntegratedLightmapBaker(std::shared_ptr<IntegratedBakeState> state, LightmapBakerConfiguration configuration);
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

// Borrows an initialized draw Engine and explicit scene/environment; no second
// renderer. One live baker and one prepared/in-flight bake frame per context.
// Different presentation slots remain usable; the caller's fence completion is
// explicit. Exclude editor-only nodes from sceneRoot or disable their tracing.
class BG2E_API IntegratedBakerContext final : public BakerContext {
  public:
    IntegratedBakerContext(draw::Engine& engine, BakerContextConfiguration configuration);
    ~IntegratedBakerContext() override;

    const BakerContextConfiguration& configuration() const override;
    uint64_t sourceGeneration() const override;
    std::shared_ptr<IntegratedLightmapBaker> createBaker(LightmapBakerConfiguration configuration);

    // Call once outside scopes on a completed acquired slot before baker.record.
    // serial is globally increasing for this context, independent of slot/sample.
    void prepareFrame(const BakeFrame& frame);
    void prepareFrame(const draw::FrameContext& frame);
    // Only after submission/GPU completion; commits counts and validates final output.
    void completeFrame(uint32_t slot, uint64_t serial);
    // Only after resetting/discarding the unsubmitted command buffer. Never use
    // this for submitted work; completeFrame must retire submitted commands.
    void abandonFrame(uint32_t slot, uint64_t serial);

    void setEnvironmentResources(std::shared_ptr<const draw::environment::EnvironmentResources> environment);

    void invalidateSources() override; // Stop producers; no open bake frame.
    void cleanup() override; // Retire/abandon open frame first, Device still alive.

  private:
    std::shared_ptr<IntegratedBakeState> _state;
};
}
