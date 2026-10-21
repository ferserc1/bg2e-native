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

namespace bg2e::bake {
struct StandaloneBakeState;
class StandaloneBakerContext;

// One exclusive target job. Context, target and returned GPU owners borrow the
// Engine/Device lifetime. All methods run on the stopped producer thread.
class BG2E_API StandaloneLightmapBaker final : public LightmapBaker {
  public:
    ~StandaloneLightmapBaker() override;

    const LightmapBakerConfiguration& configuration() const override;
    BakeProgress progress() const override;
    const LightmapSettings& frozenSettings() const;
    std::shared_ptr<const BakeSample> latestSample() const;
    std::optional<LightmapResult> result() const override;
    LightmapPixels readPixels() const override;

    // One synchronous completed sample. Complete/cancelled/failed/stale jobs reject
    // further updates. Reset freezes live settings again within the same scene.
    void update();
    void resetAccumulation() override;
    void cancel() override;
    void cleanup() override;

    // Satisfies the generic baker contract; only the exact context-owned command
    // inside update() is accepted. External command recording is not supported.
    void record(const BakeFrame& frame) override;

  private:
    friend class StandaloneBakerContext;
    StandaloneLightmapBaker(std::shared_ptr<StandaloneBakeState> state,
        LightmapBakerConfiguration configuration);
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
}
