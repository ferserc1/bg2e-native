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

namespace bg2e {
namespace gpu {

CleanupManager::CleanupManager(gpu::Surface* surface)
    : _surface(surface)
{
}

void CleanupManager::push(const std::shared_ptr<DeviceResource>& resource)
{
    _resources.push_back(resource);
}

void CleanupManager::push(std::shared_ptr<DeviceResource>&& resource)
{
    _resources.push_back(std::move(resource));
}

void CleanupManager::pushStatic(const std::shared_ptr<DeviceResource>& resource)
{
    _staticResources.push_back(resource);
}

void CleanupManager::pushStatic(std::shared_ptr<DeviceResource>&& resource)
{
    _staticResources.push_back(std::move(resource));
}

void CleanupManager::flush()
{
    // Static resources first, in insertion order
    for (auto& resource : _staticResources)
    {
        if (resource)
        {
            resource->cleanup();
        }
    }
    _staticResources.clear();

    // Normal resources after, in reverse insertion order
    for (auto it = _resources.rbegin(); it != _resources.rend(); ++it)
    {
        if (*it)
        {
            (*it)->cleanup();
        }
    }
    _resources.clear();
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
    _deferredCleanups.push_back({
        _surface->frameCounter() + _surface->inFlightFrames(),
        std::move(cleanup)
    });
}

void CleanupManager::flushDeferred()
{
    auto counter = _surface->frameCounter();
    _deferredCleanups.erase(
        std::remove_if(_deferredCleanups.begin(), _deferredCleanups.end(),
            [counter](const DeferredCleanup& d) {
                if (d.targetFrame <= counter) {
                    d.cleanup();
                    return true;
                }
                return false;
            }),
        _deferredCleanups.end()
    );
}

void CleanupManager::flushAllDeferred()
{
    for (auto& d : _deferredCleanups)
    {
        d.cleanup();
    }
    _deferredCleanups.clear();
}

}
}
