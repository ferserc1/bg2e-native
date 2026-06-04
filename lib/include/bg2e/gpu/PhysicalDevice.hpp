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

#include <optional>
#include <vector>
#include <memory>
#include <string>

namespace bg2e {
namespace gpu {

class Instance;
class Surface;

struct RayTracingCapabilities {
    bool available = false;
    bool rayTracingPipeline = false;
    bool rayQuery = false;
    bool accelerationStructure = false;
    bool bufferDeviceAddress = false;

    inline bool fullSupported() const
    {
        return available &&
            rayTracingPipeline &&
            rayQuery &&
            accelerationStructure &&
            bufferDeviceAddress;
    }
};

struct PhysicalDeviceProperties {
    enum DeviceType {
        IntegratedGPU,
        DiscreteGPU,
        VirtualGPU,
        CPU,
        Other
    } deviceType = Other;

    size_t totalHeapMemoryMB = 0;
    DeviceType type = Other;
    std::string name;
    uint32_t vendor = 0;
    uint32_t id = 0;

    RayTracingCapabilities rayTracing;

    uint32_t getScore() const;
    bool rayTracingSupported() const;

    static PhysicalDeviceProperties* create();

protected:
    PhysicalDeviceProperties() = default;
};

class BG2E_API PhysicalDevice {
public:
    virtual ~PhysicalDevice() = default;

    static void listSuitableDevices(
        const Instance* instance,
        const Surface* surface,
        std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
    );

    virtual void choose(Instance& instance, Surface& surface) = 0;

    virtual bool isValid() const = 0;

    virtual const std::shared_ptr<PhysicalDeviceProperties> properties() const = 0;
};

}
}
