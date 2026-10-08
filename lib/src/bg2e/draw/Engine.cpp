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

#include <bg2e/draw/Engine.hpp>

#include <bg2e/gpu/Backend.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/CleanupManager.hpp>

#include <stdexcept>

namespace bg2e {
namespace draw {

struct Engine::Impl {
    SDL_Window* window = nullptr;               // Non-owning
    gpu::Backend* backend = nullptr;            // Non-owning
    gpu::Instance* instance = nullptr;          // Non-owning (shared with Backend)
    std::unique_ptr<gpu::PhysicalDevice> physicalDevice;
    std::unique_ptr<gpu::Device> device;
    std::unique_ptr<gpu::WindowSurface> surface;
    std::unique_ptr<gpu::CleanupManager> cleanupManager;
    EngineConfig config;
    bool initialized = false;

    void reset()
    {
        cleanupManager.reset();
        surface.reset();
        device.reset();
        physicalDevice.reset();
        instance = nullptr;
        backend = nullptr;
        window = nullptr;
        config = EngineConfig{};
        initialized = false;
    }
};

Engine::Engine()
    : _impl { std::make_unique<Impl>() }
{
}

Engine::~Engine() = default;

void Engine::init(SDL_Window* window, gpu::Backend& backend, const EngineConfig& config)
{
    if (_impl->initialized)
    {
        throw std::logic_error("draw::Engine is already initialized.");
    }

    _impl->window = window;
    _impl->backend = &backend;
    _impl->config = config;

    throw std::logic_error("draw GPU initialization is not implemented; complete milestone 02");
}

void Engine::cleanup()
{
    // Safe for an uninitialized shell and for repeat calls.
    _impl->reset();
}

gpu::BackendType Engine::backendType() const
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return _impl->config.backend;
}

gpu::Instance* Engine::instance() const
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return _impl->instance;
}

gpu::PhysicalDevice* Engine::physicalDevice() const
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return _impl->physicalDevice.get();
}

gpu::Device* Engine::device() const
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return _impl->device.get();
}

gpu::WindowSurface* Engine::surface() const
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return _impl->surface.get();
}

gpu::CleanupManager& Engine::cleanupManager()
{
    if (!_impl->initialized)
    {
        throw std::logic_error("draw::Engine is not initialized.");
    }
    return *_impl->cleanupManager.get();
}

}
}
