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

#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/gpu/Device.hpp>
#include <stdexcept>
#include <algorithm>

namespace bg2e {
namespace gpu {

CleanupManager::CleanupManager(gpu::Surface* surface)
    : _surface(surface)
{
}

void CleanupManager::push(const std::shared_ptr<DeviceResource>& resource)
{
    if (resource != nullptr)
    {
        _resources.push_back(resource);
    }
}

void CleanupManager::push(std::shared_ptr<DeviceResource>&& resource)
{
    if (resource != nullptr)
    {
        _resources.push_back(std::move(resource));
    }
}

void CleanupManager::pushStatic(const std::shared_ptr<DeviceResource>& resource)
{
    if (resource != nullptr)
    {
        _staticResources.push_back(resource);
    }
}

void CleanupManager::pushStatic(std::shared_ptr<DeviceResource>&& resource)
{
    if (resource != nullptr)
    {
        _staticResources.push_back(std::move(resource));
    }
}

void CleanupManager::flush()
{
    // Remove ownership before callbacks so cleanup is never invoked twice.
    auto staticResources = std::move(_staticResources);
    auto resources = std::move(_resources);
    _staticResources.clear();
    _resources.clear();
    std::exception_ptr error;
    const auto cleanup = [&error](const auto& resource) {
        try { if (resource) resource->cleanup(); }
        catch (...) { if (!error) error = std::current_exception(); }
    };
    for (auto& resource : staticResources) cleanup(resource);
    for (auto it = resources.rbegin(); it != resources.rend(); ++it) cleanup(*it);
    if (error) std::rethrow_exception(error);
}

void CleanupManager::clear()
{
    _staticResources.clear();
    _resources.clear();
}

bool CleanupManager::empty() const
{
    return _staticResources.empty() && _resources.empty();
}

void CleanupManager::defer(std::function<void()>&& cleanup)
{
    if (!_surface || !_surface->_device)
        throw std::logic_error("CleanupManager::defer requires a device-backed surface");
    auto dependencies = _surface->_device->submissionState()->snapshot();
    _deferredCleanups.push_back({ std::move(dependencies), std::move(cleanup) });
}

void CleanupManager::flushDeferred()
{
    // Remove before invoking: closures may schedule further cleanup.
    std::vector<std::function<void()>> ready;
    std::exception_ptr error;
    for (auto it = _deferredCleanups.begin(); it != _deferredCleanups.end();)
    {
        bool completed = true;
        for (const auto& record : it->dependencies)
        {
            try { if (!record->completed()) completed = false; }
            catch (...) { if (!error) error = std::current_exception(); }
        }
        if (completed)
        {
            ready.push_back(std::move(it->cleanup));
            it = _deferredCleanups.erase(it);
        }
        else ++it;
    }
    for (auto& cleanup : ready) cleanup();
    if (error) std::rethrow_exception(error);
}

void CleanupManager::flushAllDeferred()
{
    auto pending = std::move(_deferredCleanups);
    _deferredCleanups.clear();
    std::exception_ptr error;
    for (auto& entry : pending)
    {
        try { entry.cleanup(); }
        catch (...) { if (!error) error = std::current_exception(); }
    }
    if (error) std::rethrow_exception(error);
}

}
}
