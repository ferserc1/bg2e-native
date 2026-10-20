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
#include <bg2e/draw/RenderLoop.hpp>
#include <stdexcept>

namespace bg2e::draw::tests {

// Reproduces a preference attachment callback changing effect quality after
// initial target allocation. No shader files or native effect implementation.
class InitializationProbeRenderer : public Renderer {
  public:
    const RendererConfiguration& configuration() const override
    {
        return _configuration;
    }

    void initialize(Engine&, RendererConfiguration configuration) override
    {
        _configuration = std::move(configuration);
        targetsStale = false;
    }
    void attachScene(std::shared_ptr<scene::Node>) override
    {
    }
    void resize(gpu::Size2D output, gpu::Size2D internal) override
    {
        _configuration.outputExtent = output;
        _configuration.internalExtent = internal;
        targetsStale = false;
        ++rebuilds;
    }
    bool needsReconfiguration() const override
    {
        return targetsStale;
    }

    void update(const FrameContext&) override
    {
        if (targetsStale)
        {
            throw std::runtime_error("First update used stale restored-quality targets");
        }
    }
    void record(const FrameContext& frame) override
    {
        if (targetsStale)
        {
            throw std::runtime_error("First record used stale restored-quality targets");
        }
        frame.commandBuffer.beginRendering(&frame.colorTarget);
        frame.commandBuffer.clearColor(0, {0, 0, 0, 1});
        frame.commandBuffer.endRendering();
    }
    void cleanup() override
    {
        _configuration = {};
    }

    bool targetsStale = false;
    uint32_t rebuilds = 0;

  private:
    RendererConfiguration _configuration;
};

// Existing initialized window Engine, stopped producers, outside an active
// loop/frame. Intentionally exercises surfaceChanged's first acquired frame.
inline void runInitializationPreferencesProbe(Engine& engine)
{
    auto renderer = std::make_unique<InitializationProbeRenderer>();
    auto* probe = renderer.get();
    auto delegate = std::make_shared<DefaultRenderLoopDelegate>(std::move(renderer));
    bool restored = false;
    delegate->setRendererChangeCallbacks({}, [&](Renderer&, uint64_t)
    {
        if (!restored)
        {
            restored = true;
            probe->targetsStale = true;
        }
    });
    RenderLoop loop;
    loop.setDelegate(delegate);
    loop.init(&engine);
    loop.initScene();
    for (uint32_t attempt = 0; attempt < 60 && loop.renderedSceneCount() == 0; ++attempt)
    {
        loop.frame(1.0f / 60.0f);
    }
    if (!restored || probe->rebuilds != 1 || loop.renderedSceneCount() != 1)
    {
        throw std::runtime_error("Startup did not rebuild restored preferences before first render");
    }
    loop.cleanup();
}

}
