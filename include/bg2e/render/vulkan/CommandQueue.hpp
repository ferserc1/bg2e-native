
#ifndef bg2e_render_vulkan_commandqueue_hpp
#define bg2e_render_vulkan_commandqueue_hpp

#include <bg2e/export.hpp>

#include <bg2e/render/CommandQueue.hpp>
#include <bg2e/render/Renderer.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_EXPORT CommandQueue : public render::CommandQueue
{
public:
    CommandQueue(render::Renderer* renderer)
        :render::CommandQueue(renderer)
    {
    }

    void beginFrame(const glm::vec4&);
    
    void endFrame();
    
    void setViewport(const Viewport&);
    
    void setScissor(int32_t x, int32_t y, const Size& size);
    
};

}
}
}

#endif

