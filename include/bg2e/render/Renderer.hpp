#ifndef bg2e_render_renderer_hpp
#define bg2e_render_renderer_hpp

#include <bg2e/export.hpp>

#include <glm/vec4.hpp>

#include <string>

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
    
    inline void setClearColor(const glm::vec4& c) { _clearColor = c; }
    inline const glm::vec4& clearColor() const { return _clearColor; }
    
protected:
    glm::vec4 _clearColor;
};

}
}

#endif
