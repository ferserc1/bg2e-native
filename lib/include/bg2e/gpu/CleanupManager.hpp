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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/Surface.hpp>

#include <deque>
#include <functional>
#include <memory>
#include <vector>

namespace bg2e {
namespace gpu {

class BG2E_API CleanupManager {
public:
    // Constructor now requires a Surface pointer for deferred cleanup timing.
    // The surface is NOT owned — the caller must ensure it outlives the manager.
    explicit CleanupManager(gpu::Surface* surface);
    ~CleanupManager() = default;

    CleanupManager(const CleanupManager&) = delete;
    CleanupManager& operator=(const CleanupManager&) = delete;

    // --- Existing API (unchanged) ---

    void push(const std::shared_ptr<DeviceResource>& resource);
    void push(std::shared_ptr<DeviceResource>&& resource);

    void pushStatic(const std::shared_ptr<DeviceResource>& resource);
    void pushStatic(std::shared_ptr<DeviceResource>&& resource);

    void flush();
    void clear();
    bool empty() const;

    // --- Deferred cleanup API (new) ---

    // Schedule a cleanup closure to run after inFlightFrames() frames have elapsed.
    // The closure will be executed when flushDeferred() is called and
    // surface->frameCounter() >= targetFrame.
    void defer(std::function<void()>&& cleanup);

    // Execute all deferred closures whose targetFrame <= surface->frameCounter().
    // Call this AFTER endFrame() in the render loop (i.e., after the fence).
    void flushDeferred();

    // Execute ALL pending deferred closures immediately, regardless of frame counter.
    // Call this after device->waitIdle() at application shutdown.
    void flushAllDeferred();

private:
    gpu::Surface* _surface;

    std::deque<std::shared_ptr<DeviceResource>> _staticResources;
    std::deque<std::shared_ptr<DeviceResource>> _resources;

    struct DeferredCleanup {
        uint64_t targetFrame;
        std::function<void()> cleanup;
    };
    std::vector<DeferredCleanup> _deferredCleanups;
};

}
}
