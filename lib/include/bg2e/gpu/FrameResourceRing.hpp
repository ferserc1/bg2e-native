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

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace bg2e {
namespace gpu {

template <typename T>
class FrameResourceRing {
    static_assert(std::is_base_of_v<DeviceResource, T>,
                  "FrameResourceRing<T>: T must derive from DeviceResource");

public:
    FrameResourceRing() = default;
    ~FrameResourceRing() = default;

    FrameResourceRing(const FrameResourceRing&) = delete;
    FrameResourceRing& operator=(const FrameResourceRing&) = delete;

    void create(
        gpu::Surface* surface,
        std::function<std::shared_ptr<T>(uint32_t)> factory)
    {
        _surface = surface;
        uint32_t count = surface->imageCount();
        _resources.resize(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            _resources[i] = factory(i);
        }
    }

    T* current()
    {
        return _resources[_surface->currentFrameIndex()].get();
    }

    const T* current() const
    {
        return _resources[_surface->currentFrameIndex()].get();
    }

    std::shared_ptr<T> currentShared()
    {
        return _resources[_surface->currentFrameIndex()];
    }

    std::shared_ptr<const T> currentShared() const
    {
        return _resources[_surface->currentFrameIndex()];
    }

    T* at(uint32_t index)
    {
        return _resources[index].get();
    }

    const T* at(uint32_t index) const
    {
        return _resources[index].get();
    }

    std::shared_ptr<T> sharedAt(uint32_t index)
    {
        return _resources[index];
    }

    std::shared_ptr<const T> sharedAt(uint32_t index) const
    {
        return _resources[index];
    }

    uint32_t size() const
    {
        return static_cast<uint32_t>(_resources.size());
    }

    bool empty() const
    {
        return _resources.empty();
    }

    void cleanup()
    {
        for (auto it = _resources.rbegin(); it != _resources.rend(); ++it)
        {
            if (*it)
            {
                (*it)->cleanup();
            }
        }
    }

    void clear()
    {
        _resources.clear();
        _surface = nullptr;
    }

private:
    gpu::Surface* _surface = nullptr;
    std::vector<std::shared_ptr<T>> _resources;
};

}
}
