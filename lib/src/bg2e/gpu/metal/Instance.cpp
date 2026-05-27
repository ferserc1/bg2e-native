/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/metal/Instance.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

Instance::Instance()
{
}

void Instance::setApplicationName(const std::string& name)
{
    _applicationName = name;
}

const std::string& Instance::applicationName() const
{
    return _applicationName;
}

void Instance::enableDebugMode(bool value)
{
    _debugMode = value;
}

bool Instance::debugModeEnabled() const
{
    return _debugMode;
}

void Instance::create(SDL_Window* window)
{
    _presentationMode = PresentationMode::Windowed;
    _window = window;
}

void Instance::create()
{
    _presentationMode = PresentationMode::Offscreen;
}

void Instance::cleanup()
{
}

void Instance::assertMetalSupport()
{
    NS::Array* devices = MTL::CopyAllDevices();

    if (!devices || devices->count() == 0)
    {
        throw std::runtime_error("No Metal compatible devices available");
    }
}

}
}
}