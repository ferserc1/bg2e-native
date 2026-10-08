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

namespace bg2e {
namespace draw {

class Engine;

// Experimental draw render loop delegate. Resources updated per in-flight
// frame belong to the objects using them; there is no central per-frame
// descriptor allocation callback.
class BG2E_API RenderLoopDelegate {
public:
    virtual ~RenderLoopDelegate() = default;

    virtual void init(Engine* engine) { _engine = engine; }

    virtual void initScene() {}

    virtual void resize(gpu::Size2D /* newExtent */) {}

    virtual void update(const FrameContext& /* frameContext */) {}

    virtual void render(const FrameContext& frameContext) = 0;

    virtual void cleanup() {}

protected:
    Engine* _engine = nullptr;
};

}
}
