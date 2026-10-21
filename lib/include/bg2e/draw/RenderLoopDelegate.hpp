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
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/gpu/Common.hpp>

#include <functional>

namespace bg2e {
namespace draw {

class Engine;

// Draw render loop delegate. Resources updated per in-flight
// frame belong to the objects using them; there is no central per-frame
// descriptor allocation callback.
class BG2E_API RenderLoopDelegate {
  public:
    virtual ~RenderLoopDelegate() = default;

    virtual void init(Engine* engine)
    {
        _engine = engine;
    }

    virtual void initScene()
    {
    }

    virtual void resize(gpu::Size2D /* newExtent */)
    {
    }

    // Called before acquisition/UI preparation/recording, including while paused.
    // Lifecycle changes may drain the device here; no command scope is active.
    virtual void prepareFrame()
    {
    }

    // The default preserves existing custom delegate resize overrides.
    virtual void surfaceChanged(gpu::Size2D extent, gpu::PixelFormat /* format */)
    {
        resize(extent);
    }

    virtual void update(const FrameContext& /* frameContext */)
    {
    }

    virtual void render(const FrameContext& frameContext) = 0;

    virtual void cleanup()
    {
    }

  protected:
    // Main-thread invalidation. Animation may call this during update/record to
    // keep the following scene frame dirty, independently of UI presentation.
    void requestSceneFrame()
    {
        if (_requestSceneFrame)
        {
            _requestSceneFrame();
        }
    }

    Engine* _engine = nullptr;

  private:
    friend class RenderLoop;
    std::function<void()> _requestSceneFrame;
};

}
}
