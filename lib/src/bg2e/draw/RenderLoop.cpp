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

#include <bg2e/draw/RenderLoop.hpp>

#include <stdexcept>

namespace bg2e {
namespace draw {

RenderLoop::RenderLoop() = default;

RenderLoop::~RenderLoop() = default;

void RenderLoop::init(Engine* engine)
{
    _engine = engine;
    throw std::logic_error("draw::RenderLoop initialization is not implemented; complete milestone 02");
}

void RenderLoop::initScene()
{
    throw std::logic_error("draw::RenderLoop scene initialization is not implemented; complete milestone 02");
}

void RenderLoop::frame(float deltaSeconds, ui::UserInterface& userInterface)
{
    throw std::logic_error("draw::RenderLoop frame execution is not implemented; complete milestone 02");
}

void RenderLoop::requestResize()
{
    _resizeRequested = true;
    _sceneDirty = true;
}

void RenderLoop::pauseScene(const glm::vec4& clearColor)
{
    _scenePaused = true;
    _sceneClearColor = clearColor;
}

void RenderLoop::resumeScene()
{
    _scenePaused = false;
    _sceneDirty = true;
}

void RenderLoop::requestSceneFrame()
{
    _sceneDirty = true;
}

void RenderLoop::cleanup()
{
    // Safe for an uninitialized shell and for repeat calls.
    if (_delegate)
    {
        _delegate->cleanup();
    }
    _engine = nullptr;
    _resizeRequested = false;
    _scenePaused = false;
    _sceneDirty = true;
    _sceneClearColor = { 0.f, 0.f, 0.f, 1.f };
}

}
}
