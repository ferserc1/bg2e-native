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

namespace bg2e {
namespace gpu {

class Instance;
class PhysicalDevice;
class Surface;
class Queue;

class BG2E_API Device {
public:
    virtual ~Device() = default;

    virtual void create(Instance* instance, PhysicalDevice* physicalDevice, Surface* surface) = 0;
    virtual void cleanup() = 0;
    virtual void waitIdle() = 0;

    virtual bool isValid() const = 0;

    virtual const Queue& graphicsQueue() const = 0;
    virtual const Queue& presentQueue() const = 0;
    virtual const Queue& transferQueue() const = 0;
};

}
}
