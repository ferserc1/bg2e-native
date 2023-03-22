
#ifndef bg2e_render_vulkan_resourcedestroymanager_hpp
#define bg2e_render_vulkan_resourcedestroymanager_hpp

#include <deque>
#include <functional>

namespace bg2e {
namespace render {
namespace vulkan {

class ResourceDestroyManager
{
public:
    void push_function(std::function<void()>&& fn)
    {
        _deletors.push_back(fn);
    }
    
    void flush()
    {
        for (auto it = _deletors.rbegin(); it != _deletors.rend(); ++it)
        {
            (*it)();
        }
        
        _deletors.clear();
    }
    
protected:
    std::deque<std::function<void()>> _deletors;
};

}
}
}

#endif

