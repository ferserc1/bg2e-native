
#include <iostream>
#include <bx/bx.h>
#include <bx/file.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <bg2e/wnd.hpp>
#include <bg2e/utils.hpp>

#include "example_shaders.h"

struct PosColorVertex
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    };

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
    0, 1, 2, // 0
    1, 3, 2,
    4, 6, 5, // 2
    5, 6, 7,
    0, 2, 4, // 4
    4, 2, 6,
    1, 5, 3, // 6
    5, 7, 3,
    0, 4, 1, // 8
    4, 5, 1,
    2, 3, 6, // 10
    6, 3, 7,
};

class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
        
    void init() {
        std::cout << "Init" << std::endl;
    
        PosColorVertex::init();
        
        _fileReader = BX_NEW(&_allocator, bx::FileReader);
        
        _vertexBuffer = bgfx::createVertexBuffer(
            bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)),
            PosColorVertex::ms_layout);
                
        _indexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));
        
        //_program = bgfx::createProgram(shaders::basic_vertex_metal,shaders::basic_fragment_metal, true);
    }
    
    void resize(uint32_t w, uint32_t h) {
        std::cout << "Resize: " << w << ", " << h << std::endl;
    }
    
    void update(float delta) {
        // std::cout << "Update: " << delta << std::endl;
        auto viewId = window()->viewId();
        
        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303960ff, 1.0f, 0.0f);
    }
    
    void draw() {
//        bgfx::dbgTextClear();
//        //bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20, bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
//        bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
//        bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
//        bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
//        bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");
//        const bgfx::Stats* stats = bgfx::getStats();
//        bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.", stats->width, stats->height, stats->textWidth, stats->textHeight);
//        // Enable stats or debug text.
//        bgfx::setDebug(_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
//        // Advance to next frame. Process submitted rendering primitives.
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
    
protected:
    bool _showStats = false;
    
    bgfx::VertexBufferHandle _vertexBuffer;
    bgfx::IndexBufferHandle _indexBuffer;
    bgfx::ProgramHandle _program;
    
    bx::DefaultAllocator _allocator;
    bx::FileReaderI * _fileReader;
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
 
