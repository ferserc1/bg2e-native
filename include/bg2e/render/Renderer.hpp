#ifndef bg2e_render_renderer_hpp
#define bg2e_render_renderer_hpp

#include <bg2e/export.hpp>

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
    virtual void destroy() = 0;
};

}
}

#endif
