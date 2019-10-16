
#include <iostream>
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <bg2e/wnd.hpp>

class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
    void init() {
        std::cout << "Init" << std::endl;
    }
    
    void resize(uint32_t w, uint32_t h) {
        std::cout << "Resize: " << w << ", " << h << std::endl;
    }
    
    void update(float delta) {
        // std::cout << "Update: " << delta << std::endl;
    }
    
    void draw() {
        bgfx::dbgTextClear();
        //bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20, bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
        bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
        bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
        bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
        bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");
        const bgfx::Stats* stats = bgfx::getStats();
        bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.", stats->width, stats->height, stats->textWidth, stats->textHeight);
        // Enable stats or debug text.
        bgfx::setDebug(_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
        // Advance to next frame. Process submitted rendering primitives.
    }
    
    void destroy() {
        std::cout << "Destroy" << std::endl;
    }
    
    void keyUp(const bg2e::wnd::KeyboardEvent & evt) {
        if (evt.keyCode() == bg2e::wnd::KeyboardEvent::KeyF1) {
            _showStats = !_showStats;
        }
        std::cout << "Key up" << std::endl;
    }
    
    void keyDown(const bg2e::wnd::KeyboardEvent & evt) {
        std::cout << "Key down" << std::endl;
    }
    
    void mouseDrag(const bg2e::wnd::MouseEvent & evt) {
        std::cout << "Mouse drag" << std::endl;
    }
    
    void mouseUp(const bg2e::wnd::MouseEvent &) {
        std::cout << "Mouse up" << std::endl;
    }
    
    void mouseDown(const bg2e::wnd::MouseEvent &) {
        std::cout << "Mouse down" << std::endl;
    }
    
    void mouseMove(const bg2e::wnd::MouseEvent &) {
        std::cout << "Mouse drag" << std::endl;
    }
    
    void mouseWheel(const bg2e::wnd::MouseEvent &) {
        std::cout << "Mouse wheel" << std::endl;
    }
    
protected:
    bool _showStats = false;
};

int main(int argc, char ** argv) {
    // Create the main loop to initialize the window system
    bg2e::wnd::MainLoop mainLoop;
    
    
    bg2e::wnd::Window * win = new bg2e::wnd::Window();
    win->registerEventHandler(new MyEventHandler());
    win->create(1024,768,"Hello world");
    
    mainLoop.registerWindow(win);
    
    return mainLoop.run();
}
 
