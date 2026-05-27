/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/Factory.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/vk/Backend.hpp>
#include <bg2e/gpu/metal/Backend.hpp>

namespace bg2e {
namespace gpu {

std::unique_ptr<Backend> Factory::_backend;

void Factory::init(BackendType type) {
    if (type == BackendType::Vulkan)
    {
        _backend = std::make_unique<vk::Backend>();
    }
    else if (type == BackendType::Metal && base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        _backend = std::make_unique<metal::Backend>();
    }
    else
    {
        throw std::runtime_error("Could not create backend. Maybe the specified backend is not available.");
    }
}

Backend* Factory::backend()
{
    if (!_backend)
    {
        throw std::runtime_error("Backend not initialized");
    }
    return _backend.get();
}

}
}
