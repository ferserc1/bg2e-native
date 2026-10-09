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
#include <bg2e/render/Engine.hpp>
#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <SDL2/SDL.h>

#include <functional>


namespace bg2e {
namespace draw { class Engine; }
namespace gpu { class CommandBuffer; class SurfaceFrame; }

namespace ui {

class BG2E_API UserInterface {
public:
    UserInterface();
    ~UserInterface();
    UserInterface(const UserInterface&) = delete;
    UserInterface& operator=(const UserInterface&) = delete;

    void init(render::Engine *);
    void init(draw::Engine *);
    bool initialized() const;

    void processEvent(SDL_Event * event);

    void newFrame();
    void newFrame(gpu::CommandBuffer&, gpu::SurfaceFrame&);
    
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);
    void draw(gpu::CommandBuffer&, gpu::SurfaceFrame&);

    void cleanup();

    void setFrameOverride(std::function<void()> fn);
    void clearFrameOverride();

    inline void setDelegate(std::shared_ptr<UserInterfaceDelegate> delegate) { _delegate = delegate; }

    static float getScale() { return s_uiScale; }
    static void setScale(float scale);

protected:
    render::Engine * _engine = nullptr;
    struct Impl;
    std::shared_ptr<Impl> _impl; // Production cleanup closures retain this state.

    std::shared_ptr<UserInterfaceDelegate> _delegate;

    std::function<void()> _frameOverride;

    void finishFrame();

    static float s_uiScale;
    static bool s_uiFontLoaded;
    static bool s_uiScaleChanged;

    static void updateScale();
};

}
}
