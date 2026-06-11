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

#pragma once

#include <deque>
#include <functional>

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Device.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API CleanupManager {
public:

    void push(std::function<void(VkDevice)>&& fn)
    {
        _deleters.push_back(fn);
    }

    // Static resources (e.g. shared caches) that must be released before
    // per-object deleters run. Called in insertion order, before _deleters.
    void pushStatic(std::function<void(VkDevice)>&& fn)
    {
        _staticDeleters.push_back(fn);
    }

    void flush(const Device& device)
    {
        for (auto& fn : _staticDeleters)
        {
            fn(device.handle());
        }
        _staticDeleters.clear();

        for (auto it = _deleters.rbegin(); it != _deleters.rend(); ++it)
        {
            (*it)(device.handle());
        }
        _deleters.clear();
    }

protected:
    std::deque<std::function<void(VkDevice)>> _staticDeleters;
    std::deque<std::function<void(VkDevice)>> _deleters;

};

}
}
}
