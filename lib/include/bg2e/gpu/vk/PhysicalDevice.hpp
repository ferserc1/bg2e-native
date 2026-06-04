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

#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <optional>
#include <vector>
#include <memory>

namespace bg2e {
namespace gpu {
namespace vk {

class PhysicalDevice : public gpu::PhysicalDevice {
public:
    void choose(gpu::Instance& instance, gpu::Surface& surface) override;

    bool isValid() const override;

    const std::shared_ptr<PhysicalDeviceProperties> properties() const override;

    VkPhysicalDevice handle() const { return _physicalDevice; }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;

        bool isComplete() const;
        bool isCompleteHeadless() const;
    };

    QueueFamilyIndices queueFamilyIndices() const;

    static const std::vector<const char*>& getRequiredDeviceExtensions(bool offscreen);

private:
    VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
    std::shared_ptr<PhysicalDeviceProperties> _properties;
    const gpu::Surface* _surface{nullptr};

    static bool isSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    static bool isSuitableHeadless(VkPhysicalDevice device);
    static bool checkDeviceExtensions(VkPhysicalDevice device);
    static PhysicalDeviceProperties* queryProperties(VkPhysicalDevice device);
    static QueueFamilyIndices queryQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE);
};

}
}
}
