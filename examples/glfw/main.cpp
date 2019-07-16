

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <iostream>

class MyDelegate : public bg2wnd::WindowDelegate {
public:
    void init() {
        std::cout << "init" << std::endl;
    }
    
    void resize(const bg2math::int2 & size) {
        std::cout << "resize: " << size.x() << ", " << size.y() << std::endl;
    }
    
    
    void update(float delta) {
       // std::cout << "udpate: " << delta << std::endl;
    }
    
    void draw() {
       // std::cout << "draw" << std::endl;
    }
    
    void cleanup() {
        std::cout << "cleanup" << std::endl;
    }
    
    void keyUp(const bg2wnd::KeyboardEvent & e) {
        if (e.shiftEnabled() && e.keyCode() == bg2wnd::KeyboardEvent::KeyE) {
            std::cout << "Shift + E up" << std::endl;
        }
    }
    
    void keyDown(const bg2wnd::KeyboardEvent & e) {
        if (e.shiftEnabled() && e.keyCode() == bg2wnd::KeyboardEvent::KeyE) {
            std::cout << "Shift + E down" << std::endl;
        }
    }
    

    void mouseMove(const bg2wnd::MouseEvent & e) {
        std::cout << "Mouse move, x=" << e.posX() << ", y=" << e.posY() << std::endl;
    }
    
    void mouseDown(const bg2wnd::MouseEvent & e) {
        std::cout << "MouseDown: B1=" << e.button(1) <<
            ", B2=" << e.button(2) <<
            ", B3=" << e.button(3) << std::endl;
    }
    
    void mouseUp(const bg2wnd::MouseEvent & e) {
        std::cout << "MouseUp: B1=" << e.button(1) <<
            ", B2=" << e.button(2) <<
            ", B3=" << e.button(3) << std::endl;
    }
    
    void mouseWheel(const bg2wnd::MouseEvent & e) {
        std::cout << "Mouse wheel: x=" << e.wheelDeltaX() << ", y=" <<
            e.wheelDeltaY() << std::endl;
    }
};

int main(int argc, char ** argv) {
    bg2wnd::Application app;
    
    auto window = bg2wnd::Window::Create();
    window->setWindowDelegate(new MyDelegate());
    app.addWindow(window);
    app.run();
    
    return 0;
}
