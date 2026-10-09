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

#include <mutex>

namespace bg2e {
namespace gpu {

namespace {
std::mutex backendMutex;

std::shared_ptr<Backend> makeBackend(BackendType type)
{
    if (type == BackendType::Vulkan) return std::make_shared<vk::Backend>();
    if (type == BackendType::Metal && base::PlatformTools::currentPlatform() == base::Platform::macOS)
        return std::make_shared<metal::Backend>();
    throw std::runtime_error("Could not create backend. Maybe the specified backend is not available.");
}
}

std::shared_ptr<Backend> Factory::_backend;

void Factory::init(BackendType type)
{
    std::lock_guard<std::mutex> lock(backendMutex);
    if (_backend && _backend.use_count() > 1)
        throw std::logic_error("Cannot replace the GPU backend while it is retained by an active execution or engine.");
    _backend = makeBackend(type);
}

Backend* Factory::backend()
{
    std::lock_guard<std::mutex> lock(backendMutex);
    if (!_backend) throw std::runtime_error("Backend not initialized");
    return _backend.get();
}

std::shared_ptr<Backend> Factory::acquireBackend(BackendType type)
{
    std::lock_guard<std::mutex> lock(backendMutex);
    if (_backend && _backend.use_count() > 1)
        throw std::logic_error("Cannot prepare a GPU backend while another execution or engine retains it.");
    _backend = makeBackend(type);
    return _backend;
}

std::shared_ptr<Backend> Factory::retainBackend(Backend& backend)
{
    std::lock_guard<std::mutex> lock(backendMutex);
    return _backend.get() == &backend ? _backend : std::shared_ptr<Backend>{};
}

}
}
