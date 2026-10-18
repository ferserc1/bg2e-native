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

#include <bg2e/bake/BakerContext.hpp>
#include <bg2e/bake/LightmapPixels.hpp>
#include <bg2e/gpu/Common.hpp>
#include <optional>
#include <string>

namespace bg2e::gpu { class Image; }
namespace bg2e::bake {
struct BG2E_API LightmapBakerConfiguration {
    std::shared_ptr<scene::Node> target;
    std::shared_ptr<LightmapSettings> settings = std::make_shared<LightmapSettings>();

    void validate() const; // Target geometry/UV/device checked by concrete preparation.
};
struct BakeProgress {
    BakeState state = BakeState::Ready;
    uint32_t completedSamples = 0, requestedSamples = 0; // Exact completed GPU updates, not UI frames/ray count.
    uint64_t sourceGeneration = 0, targetGeneration = 0;
    std::string status;
};
// Only published after completed submitted work, composition and dilation.
// Image is a sampled/storage/readback floating RGBA indirect multiplier; alpha
// is not validity. Executor declares/checks actual precision, rejects overflow
// and never clamps HDR float output. CPU pixel format is independent.
// GI multiplier = (incoming radiance/pi)/(environment irradiance/pi), channelwise;
// zero reference channels give zero. AO is replicated scalar visibility.
// Receiver albedo/PI are not applied again. Uncovered pixels outside chart dilation
// and unbaked pixels are neutral one;
// chart/validity masks are retained separately by execution, never inferred from RGB.
struct LightmapResult {
    std::shared_ptr<gpu::Image> image;
    gpu::Size2D extent;
    LightmapMode mode = LightmapMode::RTGI;
    LightmapSignal signal = LightmapSignal::IndirectMultiplierRGB;
    uint32_t uvSet = 1; // texCoord1 / UV2, separate from material AO presence.
    uint64_t sourceGeneration = 0, targetGeneration = 0;
};
// Abstract target contract. IntegratedLightmapBaker records, accumulates and
// publishes validated output only after the final GPU submission completes.
// Concrete executor must reject unsupported required effects before output and
// reject stale/duplicate BakeFrame generations/slots. Freeze settings per job.
class BG2E_API LightmapBaker {
  public:
    virtual ~LightmapBaker() = default;
    LightmapBaker(const LightmapBaker&) = delete;
    LightmapBaker& operator=(const LightmapBaker&) = delete;

    virtual const LightmapBakerConfiguration& configuration() const = 0;
    virtual BakeProgress progress() const = 0;

    virtual void resetAccumulation() = 0; // New target generation, no stale result publication.
    virtual void cancel() = 0; // Retain submitted owners until completion.
    virtual void cleanup() = 0; // Stopped boundary; Device must still be alive.

    virtual void record(const BakeFrame& frame) = 0;

    virtual std::optional<LightmapResult> result() const = 0; // Empty unless current complete result.
    virtual LightmapPixels readPixels() const = 0; // Throw if missing/incomplete/stale.

  protected:
    LightmapBaker() = default;
};
}
