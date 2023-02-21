
#ifndef bg2e_app_appcontroller_hpp
#define bg2e_app_appcontroller_hpp 

#include <memory>
#include <bg2e/app/KeyboardEvent.hpp>
#include <bg2e/app/MouseEvent.hpp>

namespace bg2e {
namespace app {

class Window;
class AppController {
    friend Window;
public:
    
    void init() {}
    
    virtual void resize(uint32_t w, uint32_t h) {}
    virtual void frame(float delta) {}
    virtual void display() {}
    virtual void destroy() {}
    virtual void keyDown(const KeyboardEvent& evt) {}
    virtual void keyUp(const KeyboardEvent& evt) {}
    virtual void mouseUp(const MouseEvent& evt) {}
    virtual void mouseDown(const MouseEvent& evt) {}
    virtual void mouseMove(const MouseEvent& evt) {}
    virtual void mouseOut(const MouseEvent& evt) {}
    virtual void mouseDrag(const MouseEvent& evt) {}
    virtual void mouseWheel(const MouseEvent& evt) {}
    
    inline Window& window() { return *_window; }

private:
    Window* _window;
};

}
}

#endif
