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

    void flush(const Device& device)
    {
        for (auto it = _deleters.rbegin(); it != _deleters.rend(); ++it)
        {
            (*it)(device.handle());
        }
        _deleters.clear();
    }

protected:
    std::deque<std::function<void(VkDevice)>> _deleters;

};

}
}
}
