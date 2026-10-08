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
#include <bg2e/draw/RenderLoopDelegate.hpp>

#include <glm/glm.hpp>

#include <memory>

namespace bg2e {

namespace ui {
class UserInterface;
}

namespace draw {

class Engine;

// Experimental draw render loop. Coordinates acquisition, delegate work, UI
// composition and presentation. Owns the coordination state and, from
// milestone 02 on, the retained scene target. The engine does not own the UI.
class BG2E_API RenderLoop {
public:
    RenderLoop();
    ~RenderLoop();

    RenderLoop(const RenderLoop&) = delete;
    RenderLoop& operator=(const RenderLoop&) = delete;

    inline void setDelegate(std::shared_ptr<RenderLoopDelegate> delegate) { _delegate = delegate; }
    inline std::shared_ptr<RenderLoopDelegate> delegate() const { return _delegate; }

    void init(Engine* engine);

    void initScene();

    void frame(float deltaSeconds, ui::UserInterface& userInterface);

    void requestResize();

    void pauseScene(const glm::vec4& clearColor = { 0.f, 0.f, 0.f, 1.f });
    void resumeScene();
    inline bool isScenePaused() const { return _scenePaused; }

    // Marks the retained scene image as outdated. The next frame will run the
    // scene delegate update/render even if presentation continues meanwhile.
    void requestSceneFrame();
    inline bool sceneDirty() const { return _sceneDirty; }

    void cleanup();

protected:
    Engine* _engine = nullptr;
    std::shared_ptr<RenderLoopDelegate> _delegate;

    bool _resizeRequested = false;
    bool _scenePaused = false;
    bool _sceneDirty = true;
    glm::vec4 _sceneClearColor { 0.f, 0.f, 0.f, 1.f };
};

}
}
