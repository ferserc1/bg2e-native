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

#include <bg2e/common.hpp>
#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/gpu/Common.hpp>

#include <bg2e/draw/SettingsCodec.hpp>
#include <cstdint>
#include <optional>

namespace bg2e::draw {
class Engine;
}

namespace bg2e::gpu {
class OffscreenSurface;
}

namespace bg2e::app {

class Preferences;

struct DrawOffscreenApplicationConfig {
    // GPU backend, debug mode and target formats. No window is created.
    draw::EngineConfig engine;
    gpu::Size2D extent{1920, 1080};
    bool createDepthImage = true;
    // Frozen scene by default. Later frames advance by this fixed step, in seconds.
    float timeStepSeconds = 0.0f;
    uint32_t frames = 1; // Exact completed scene samples for the default delegate.
    std::filesystem::path graphicsConfig;
    // Borrowed opt-in persistence. An explicit graphicsConfig bypasses it.
    Preferences* persistedDefaults = nullptr;
    draw::UnknownSettingPolicy unknownSettings = draw::UnknownSettingPolicy::Reject;
    std::optional<gpu::Size2D> internalExtentOverride;
    std::optional<float> scaleOverride;
    // Parsed by the driver before GPU allocation; callers normally supply a path.
    std::shared_ptr<json::JsonNode> graphicsDocument;
};

// Native-free delegate contract for draw-based offscreen applications. The
// command and images in FrameContext are borrowed for the render callback only.
class BG2E_API DrawOffscreenApplicationDelegate {
  public:
    virtual ~DrawOffscreenApplicationDelegate() = default;

    virtual void initConfig(int /* argc */, char** /* argv */, DrawOffscreenApplicationConfig& /* config */)
    {
    }

    virtual void configure(const DrawOffscreenApplicationConfig& /* config */)
    {
    }

    virtual void init(draw::Engine* /* engine */, gpu::OffscreenSurface* /* surface */)
    {
    }

    virtual void initScene()
    {
    }

    virtual void resize(gpu::Size2D /* extent */)
    {
    }

    // Stopped boundary before acquiring/recording the next scene frame.
    virtual void prepareFrame()
    {
    }

    virtual void update(float /* deltaSeconds */, uint64_t /* sceneFrameNumber */)
    {
    }

    virtual void render(const draw::FrameContext& frameContext) = 0;

    // Internal completion boundary, before result conversion and user notification.
    virtual void completeFrame(uint64_t /* sceneFrameNumber */)
    {
    }

    virtual bool needsResolve() const
    {
        return false;
    }

    // Optional final-only submission, using the last completed scene slot.
    virtual void resolveFrame(const draw::FrameContext& /* frame */)
    {
    }

    // Called only after the optional final submission completes successfully.
    virtual void completeResolve()
    {
    }

    virtual void finish()
    {
    }

    // Called after the recorded scene and any final conversion complete on the GPU.
    virtual void didRenderFrame(uint64_t /* sceneFrameNumber */, double /* elapsedMs */)
    {
    }

    // Checked before every frame, including the first; false means no submission.
    virtual bool continueRendering() const = 0;

    virtual void cleanup()
    {
    }
};

}
