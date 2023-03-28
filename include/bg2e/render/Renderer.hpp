#ifndef bg2e_render_renderer_hpp
#define bg2e_render_renderer_hpp

#include <bg2e/export.hpp>

#include <bg2e/render/CommandQueue.hpp>

#include <glm/vec4.hpp>

#include <string>
#include <memory>
#include <functional>

namespace bg2e {

namespace app {

class Window;

}

namespace render {

class BG2E_EXPORT Renderer {
public:
    Renderer();
    virtual ~Renderer();
    
    virtual void init(const std::string& appName) = 0;
    virtual void bindWindow(app::Window&) = 0;
    virtual void resize(uint32_t w, uint32_t h) = 0;
    virtual void update(float delta) = 0;
    virtual void drawFrame() = 0;
    virtual void destroy() = 0;
    
    virtual CommandQueue* commandQueue() = 0;
        
    inline const Size& windowSize() const { return _windowSize; }
    
    inline void setDrawFunction(std::function<void(CommandQueue*)>&& drawFunc) { _drawFunction = drawFunc; }
    
protected:
    Size _windowSize;
    std::shared_ptr<CommandQueue> _commandQueue;
    std::function<void(CommandQueue*)> _drawFunction;
};

}
}

#endif
