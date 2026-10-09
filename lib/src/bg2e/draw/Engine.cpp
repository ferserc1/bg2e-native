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

#include <bg2e/gpu/Factory.hpp>

#include <exception>
#include <stdexcept>

namespace bg2e {
namespace draw {

struct Engine::Impl {
    SDL_Window* window = nullptr;               // Non-owning
    std::shared_ptr<gpu::Backend> backendLease; // Pins factory-owned backends.
    gpu::Backend* backend = nullptr;            // Non-owning
    gpu::Instance* instance = nullptr;          // Non-owning (shared with Backend)
    std::unique_ptr<gpu::PhysicalDevice> physicalDevice;
    std::unique_ptr<gpu::Device> device;
    std::unique_ptr<gpu::WindowSurface> surface;
    std::unique_ptr<gpu::CleanupManager> cleanupManager;
    EngineConfig config;
    bool initialized = false;
    bool instanceCreationStarted = false;
    bool deviceCreationStarted = false;
    bool cleaning = false;

    void reset()
    {
        cleanupManager.reset();
        surface.reset();
        device.reset();
        physicalDevice.reset();
        instance = nullptr;
        backend = nullptr;
        backendLease.reset();
        instanceCreationStarted = false;
        deviceCreationStarted = false;
        cleaning = false;
        window = nullptr;
        config = EngineConfig{};
        initialized = false;
    }
};

Engine::Engine()
    : _impl { std::make_unique<Impl>() }
{
}

Engine::~Engine()
{
    try { cleanup(); } catch (...) { /* Explicit cleanup reports shutdown errors. */ }
}

void Engine::init(SDL_Window* window, gpu::Backend& backend, const EngineConfig& config)
{
    if (_impl->initialized || _impl->instanceCreationStarted || _impl->cleaning)
        throw std::logic_error("draw::Engine is already initialized or changing lifecycle state.");
    if (!window) throw std::invalid_argument("draw::Engine::init requires an SDL window.");
    if (backend.backendType() != config.backend)
        throw std::invalid_argument("draw::Engine backend does not match EngineConfig::backend.");

    try
    {
        _impl->backendLease = gpu::Factory::retainBackend(backend);
        _impl->window = window;
        _impl->backend = &backend;
        _impl->config = config;
        _impl->instance = backend.sharedInstance();
        if (!_impl->instance) throw std::runtime_error("Draw backend returned no Instance.");
        // The backend owns the wrapper; this Engine owns its initialized lifetime.
        // Do not overwrite an instance used by another context.
        if (_impl->instance->presentationMode() != gpu::PresentationMode::Undefined)
            throw std::logic_error("Draw backend shared Instance is already in use.");
        _impl->instanceCreationStarted = true;
        _impl->instance->setApplicationName(config.applicationName);
        _impl->instance->enableDebugMode(config.debug);
        _impl->instance->create(window);

        _impl->surface = backend.createWindowSurface(_impl->instance, config.colorFormat, config.depthFormat);
        if (!_impl->surface)
            throw std::runtime_error("Draw backend returned no WindowSurface.");
        _impl->physicalDevice = backend.createPhysicalDevice();
        if (!_impl->physicalDevice) throw std::runtime_error("Draw backend returned no PhysicalDevice.");
        _impl->physicalDevice->choose(*_impl->instance, *_impl->surface);
        if (!_impl->physicalDevice->isValid()) throw std::runtime_error("No suitable draw GPU device.");
        _impl->device = backend.createDevice();
        if (!_impl->device) throw std::runtime_error("Draw backend returned no Device.");
        _impl->deviceCreationStarted = true;
        // Device::create also creates the surface render target.
        _impl->device->create(_impl->instance, _impl->physicalDevice.get(), _impl->surface.get());
        if (!_impl->device->isValid()) throw std::runtime_error("Draw GPU Device creation failed.");
        // Vulkan surface validity includes its swapchain, created by Device.
        if (!_impl->surface->isValid()) throw std::runtime_error("Draw surface render target creation failed.");
        _impl->cleanupManager = std::make_unique<gpu::CleanupManager>(_impl->surface.get());
        _impl->initialized = true;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try { cleanup(); } catch (...) { /* Preserve the initialization failure. */ }
        std::rethrow_exception(error);
    }
}

void Engine::cleanup()
{
    if (_impl->cleaning) return;
    _impl->cleaning = true;
    std::exception_ptr error;
    const auto attempt = [&error](auto&& operation) {
        try { operation(); } catch (...) { if (!error) error = std::current_exception(); }
    };
    // Callers stop frame/background producers before entering this method and
    // keep them stopped: waitIdle's submission gate reopens on return.
    if (_impl->deviceCreationStarted && _impl->device->isValid())
        attempt([&] { _impl->device->waitIdle(); });
    if (_impl->cleanupManager)
    {
        attempt([&] { _impl->cleanupManager->flushAllDeferred(); });
        attempt([&] { _impl->cleanupManager->flush(); });
        _impl->cleanupManager.reset();
    }
    // Keep the device alive while surfaces release frame command wrappers,
    // depth targets, swapchain images and presentation synchronization.
    if (_impl->surface) attempt([&] { _impl->surface->cleanup(); });
    _impl->surface.reset();
    if (_impl->deviceCreationStarted) attempt([&] { _impl->device->cleanup(); });
    _impl->device.reset();
    _impl->physicalDevice.reset();
    if (_impl->instanceCreationStarted) attempt([&] { _impl->instance->cleanup(); });
    _impl->reset();
    if (error) std::rethrow_exception(error);
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
