#pragma once

#include <deque>
#include <functional>

#include <bg2e/render/vulkan/common.hpp>

namespace bg2e {
namespace render {

class BG2E_API CleanupManager {
public:

    void push(std::function<void(VkDevice)>&& fn)
    {
        _deleters.push_back(fn);
    }

    void flush(VkDevice device)
    {
        for (auto it = _deleters.rbegin(); it != _deleters.rend(); ++it)
        {
            (*it)(device);
        }
        _deleters.clear();
    }

protected:
    std::deque<std::function<void(VkDevice)>> _deleters;

};

}
}