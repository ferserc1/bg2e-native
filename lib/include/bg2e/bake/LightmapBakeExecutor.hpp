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
#include <bg2e/bake/UvSurfacePass.hpp>
#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>

namespace bg2e::bake {
// Raw single-sample signal, not a composed LightmapResult. AO: R16 visibility;
// GI: RGBA16 incoming diffuse radiance/pi, alpha validity. Retain until completion.
struct BakeSample {
    UvSurfaceResult surface;
    std::shared_ptr<gpu::Image> signal;
    std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot;
    std::shared_ptr<const draw::environment::EnvironmentResources> environment;
    draw::LightUniforms lights;
    uint32_t sampleIndex = 0;
    LightmapMode mode = LightmapMode::RTGI;
};
// Exclusive, Device-backed recording primitive. No submission, accumulation,
// renderer history, composition or chart-unsafe denoising. Use stopped boundaries
// for initialize/cleanup and immutable inputs through completion.
class BG2E_API LightmapBakeExecutor {
  public:
    explicit LightmapBakeExecutor(BakerContextConfiguration configuration);
    ~LightmapBakeExecutor();
    LightmapBakeExecutor(const LightmapBakeExecutor&) = delete;
    LightmapBakeExecutor& operator=(const LightmapBakeExecutor&) = delete;

    void initialize(const LightmapSettings& settings);
    void cleanup();

    std::shared_ptr<const UvSurfaceGeometry> prepareGeometry(std::shared_ptr<draw::Drawable> drawable);
    std::shared_ptr<draw::pipeline::TracingSnapshot> prepareFrame(uint32_t slot, uint64_t serial,
        const draw::RenderQueue& queue);
    BakeSample record(const UvSurfaceInput& input, uint32_t sampleIndex,
        std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot, const draw::LightUniforms& lights);

    const LightmapSettings& settings() const;

  private:
    BakerContextConfiguration _configuration;
    LightmapSettings _settings;
    std::shared_ptr<UvSurfacePass> _surface;
    std::shared_ptr<draw::pipeline::RTAmbientOcclusion> _ao;
    std::shared_ptr<draw::pipeline::RTGlobalIllumination> _gi;
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false, recorded = false;
        std::shared_ptr<draw::pipeline::TracingSnapshot> snapshot;
        std::shared_ptr<BakeSample> sample;
    };
    std::vector<Slot> _slots;
    bool _initialized = false;
};
}
