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

#include <deque>
#include <memory>

namespace bg2e {
namespace gpu {

class BG2E_API CleanupManager {
public:
    CleanupManager() = default;
    ~CleanupManager() = default;

    CleanupManager(const CleanupManager&) = delete;
    CleanupManager& operator=(const CleanupManager&) = delete;

    void push(const std::shared_ptr<DeviceResource>& resource);
    void push(std::shared_ptr<DeviceResource>&& resource);

    void pushStatic(const std::shared_ptr<DeviceResource>& resource);
    void pushStatic(std::shared_ptr<DeviceResource>&& resource);

    void flush();
    void clear();
    bool empty() const;

private:
    std::deque<std::shared_ptr<DeviceResource>> _staticResources;
    std::deque<std::shared_ptr<DeviceResource>> _resources;
};

}
}
