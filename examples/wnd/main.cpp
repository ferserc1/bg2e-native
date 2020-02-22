

#include <bg2engine.hpp>

#include <iostream>
#include <array>
#include <chrono>
#include <thread>

class MyWindowDelegate : public bg2wnd::WindowDelegate {
public:
	MyWindowDelegate() {}

	void init() {
        std::cout << window()->title() << ": Init" << std::endl;
    }

    void resize(const bg2math::int2 & size) {
        std::cout << window()->title() << ": Resize (" << size.width() << "," << size.height() << ")" << std::endl;
    }

    void update(float delta) {
        //std::cout << window()->title() << ": Update (" << delta * 1000.0f << "ms)" << std::endl;
    }

    void draw() {
        //std::cout << window()->title() << ": Draw" << std::endl;
    }

    void cleanup() {
        std::cout << window()->title() << ": Cleanup" << std::endl;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {
        std::cout << window()->title() << ": KeyUp - " << e.character();
        printKeyEvent(e);
    }

    void charPress(const bg2wnd::KeyboardEvent & e) {}

    void keyDown(const bg2wnd::KeyboardEvent & e) {
        std::cout << window()->title() << ": KeyDown - " << e.character();
        printKeyEvent(e);
    }

    void mouseMove(const bg2wnd::MouseEvent & e) {
        std::cout << window()->title() << ": MouseMove - " << e.posX() << "," << e.posY() << std::endl;
    }

    void mouseDown(const bg2wnd::MouseEvent & e) {
        std::cout << window()->title() << ": MouseDown - "
            << "Button: " << eventButtonString(e) << " - "
            << e.posX() << "," << e.posY() << std::endl;
    }

    void mouseUp(const bg2wnd::MouseEvent & e) {
        std::cout << window()->title() << ": MouseUp - " 
            << "Button: " << eventButtonString(e) << " - "
            << e.posX() << "," << e.posY() << std::endl;
    }

    void mouseWheel(const bg2wnd::MouseEvent & e) {
        std::cout << window()->title() << ": MouseWheel - "
            << e.posX() << "," << e.posY() << 
            ", delta=" << e.wheelDeltaX() << "," << e.wheelDeltaY() << std::endl;
    }

    inline std::string eventButtonString(const bg2wnd::MouseEvent & e) {
        switch (e.eventButton()) {
        case bg2wnd::MouseButton::ButtonLeft:
            return "Left";
        case bg2wnd::MouseButton::ButtonMiddle:
            return "Middle";
        case bg2wnd::MouseButton::ButtonRight:
            return "Right";
        case bg2wnd::MouseButton::Button4:
            return "4";
        case bg2wnd::MouseButton::Button5:
            return "5";
        default:
            return "None";
        }
    }

    inline void printKeyEvent(const bg2wnd::KeyboardEvent & e) const {
        std::cout << " " <<
            "alt:" << (e.altEnabled() ? "true" : "false") << ", " <<
            "ctrl:" << (e.controlEnabled() ? "true" : "false") << ", " <<
            "shift:" << (e.shiftEnabled() ? "true" : "false") << ", " <<
            "capsLock:" << (e.capsLockEnabled() ? "true" : "false") << std::endl;
    }
};

int main(int argc, char ** argv) {
	bg2wnd::Application * app = bg2wnd::Application::Get();

    auto window = bg2wnd::Window::New();
    window->setWindowDelegate(new MyWindowDelegate());
    window->setTitle("Window 1");
    window->setPosition({ 40, 40, });
    window->setSize({ 800, 600 });
    app->addWindow(window);

	auto win2 = bg2wnd::Window::New();
    win2->setTitle("Window 2");
    win2->setPosition({ 110, 110 });
    win2->setSize({ 640, 480 });
	win2->setWindowDelegate(new MyWindowDelegate());
	app->addWindow(win2);

    return app->run();
}
