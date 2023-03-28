#ifndef bg2e_render_commandqueue_hpp
#define bg2e_render_commandqueue_hpp

#include <bg2e/types.hpp>

#include <glm/vec4.hpp>

#include <memory>

namespace bg2e {
namespace render {

class Renderer;

class CommandQueue {
public:
    CommandQueue(Renderer* renderer) :_renderer{renderer} {}
    virtual ~CommandQueue() {}
    
    template <class T>
    inline T* renderer() const
    {
        return dynamic_cast<T*>(_renderer);
    }
    
    inline Renderer* renderer() const {
        return _renderer;
    }
    
    virtual void beginFrame(const glm::vec4&) = 0;
    
    virtual void endFrame() = 0;
    
    virtual void setViewport(const Viewport&) = 0;
    
    virtual void setScissor(int32_t x, int32_t y, const Size& size) = 0;
    
protected:
    Renderer* _renderer;
};

}
}

#endif

