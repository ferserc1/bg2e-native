
#include <iostream>
#include <bx/bx.h>
#include <bx/file.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>

#include <bg2e/wnd.hpp>
#include <bg2e/utils.hpp>
#include <bg2e/math.hpp>

#if BG2E_PLATFORM_WINDOWS==1
#include "win64/example_shaders.h"
#elif BG2E_PLATFORM_OSX==1
#include "osx/example_shaders.h"
#elif BG2E_PLATFORM_LINUX==1
#include "linux/example_shaders.h"
#endif

struct PosColorVertex
{
    //float m_x;
    //float m_y;
    //float m_z;
	bg2e::math::float3 _position;
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

struct PosNormalVertex {
	bg2e::math::float3 _position;
	bg2e::math::float3 _normal;
	bg2e::math::float2 _tex0Coord;

	static void init() {
		ms_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosNormalVertex::ms_layout;

static PosColorVertex s_cubeVertices[] =
{
	{{ -1.0f,  1.0f,  1.0f }, 0xff000000 },
    {{  1.0f,  1.0f,  1.0f }, 0xff0000ff },
    {{ -1.0f, -1.0f,  1.0f }, 0xff00ff00 },
    {{  1.0f, -1.0f,  1.0f }, 0xff00ffff },
    {{ -1.0f,  1.0f, -1.0f }, 0xffff0000 },
    {{  1.0f,  1.0f, -1.0f }, 0xffff00ff },
    {{ -1.0f, -1.0f, -1.0f }, 0xffffff00 },
    {{  1.0f, -1.0f, -1.0f }, 0xffffffff },
};

static PosNormalVertex s_cubeNVertices[] =
{
	// front
	{{ -1.0f,  1.0f,  1.0f }, { 0.0, 0.0, 1.0 }, { 0.0f, 0.0f } },
	{{  1.0f,  1.0f,  1.0f }, { 0.0, 0.0, 1.0 }, { 1.0f, 0.0f } },
	{{  1.0f, -1.0f,  1.0f }, { 0.0, 0.0, 1.0 }, { 1.0f, 1.0f } },
	{{ -1.0f, -1.0f,  1.0f }, { 0.0, 0.0, 1.0 }, { 0.0f, 1.0f } },

	// back
	{{  1.0f,  1.0f, -1.0f }, { 0.0, 0.0, -1.0 }, { 0.0f, 0.0f } },
	{{ -1.0f,  1.0f, -1.0f }, { 0.0, 0.0, -1.0 }, { 1.0f, 0.0f } },
	{{ -1.0f, -1.0f, -1.0f }, { 0.0, 0.0, -1.0 }, { 1.0f, 1.0f } },
	{{  1.0f, -1.0f, -1.0f }, { 0.0, 0.0, -1.0 }, { 0.0f, 1.0f } },

	// left
	{{  -1.0f,  1.0f, -1.0f }, { -1.0, 0.0, 0.0 }, { 0.0f, 0.0f } },
	{{  -1.0f,  1.0f,  1.0f }, { -1.0, 0.0, 0.0 }, { 1.0f, 0.0f } },
	{{  -1.0f, -1.0f,  1.0f }, { -1.0, 0.0, 0.0 }, { 1.0f, 1.0f } },
	{{  -1.0f, -1.0f, -1.0f }, { -1.0, 0.0, 0.0 }, { 0.0f, 1.0f } },
	
	// right
	{{   1.0f,  1.0f,  1.0f }, { 1.0, 0.0, 0.0 }, { 0.0f, 0.0f } },
	{{   1.0f,  1.0f, -1.0f }, { 1.0, 0.0, 0.0 }, { 1.0f, 0.0f } },
	{{   1.0f, -1.0f, -1.0f }, { 1.0, 0.0, 0.0 }, { 1.0f, 1.0f } },
	{{   1.0f, -1.0f,  1.0f }, { 1.0, 0.0, 0.0 }, { 0.0f, 1.0f } },

	// top
	{{  -1.0f,  1.0f, -1.0f }, { 0.0, 1.0, 0.0 }, { 0.0f, 0.0f } },
	{{   1.0f,  1.0f, -1.0f }, { 0.0, 1.0, 0.0 }, { 1.0f, 0.0f } },
	{{   1.0f,  1.0f,  1.0f }, { 0.0, 1.0, 0.0 }, { 1.0f, 1.0f } },
	{{  -1.0f,  1.0f,  1.0f }, { 0.0, 1.0, 0.0 }, { 0.0f, 1.0f } },

	// bottom
	{{  -1.0f, -1.0f,  1.0f }, { 0.0, -1.0, 0.0 }, { 0.0f, 0.0f } },
	{{   1.0f, -1.0f,  1.0f }, { 0.0, -1.0, 0.0 }, { 1.0f, 0.0f } },
	{{   1.0f, -1.0f, -1.0f }, { 0.0, -1.0, 0.0 }, { 1.0f, 1.0f } },
	{{  -1.0f, -1.0f, -1.0f }, { 0.0, -1.0, 0.0 }, { 0.0f, 1.0f } }
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

static const uint16_t s_cubeTriList2[] =
{
	0, 1, 2, // front
	2, 3, 0,
	
	// back
	4, 5, 6,
	6, 7, 4,

	// left
	8, 9, 10,
	10, 11, 8,

	// right
	12, 13, 14,
	14, 15, 12,

	// top
	16, 17, 18,
	18, 19, 16,

	// bottom
	20, 21, 22,
	22, 23, 20
};

class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
        
    void init() {
        std::cout << "Init" << std::endl;
    
        PosColorVertex::init();
		PosNormalVertex::init();
        
        _vertexBuffer = bgfx::createVertexBuffer(
            bgfx::makeRef(s_cubeNVertices, sizeof(s_cubeNVertices)),
            PosNormalVertex::ms_layout);
                
        _indexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList2, sizeof(s_cubeTriList2)));
        
        bgfx::RendererType::Enum type = bgfx::getRendererType();

        //bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::basic_vertex");
        //bgfx::ShaderHandle fsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::basic_fragment");
		bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::phong_vertex");
		bgfx::ShaderHandle fsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::phong_fragment");

        // Create program from shaders.
        _program = bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);

		_lightPositionHandle = bgfx::createUniform("lightPosition", bgfx::UniformType::Vec4);
		_normalMatHandle = bgfx::createUniform("u_normal", bgfx::UniformType::Mat4);
    }
    
    static const bgfx::EmbeddedShader _shaders[];
    
    void resize(uint32_t w, uint32_t h) {
        std::cout << "Resize: " << w << ", " << h << std::endl;
        bgfx::setViewRect(window()->viewId(), 0, 0, window()->width(), window()->height());
    }
    
    void update(float delta) {
        auto viewId = window()->viewId();
        
        const bg2e::math::float3 at = { 0.0f, 0.0f, 0.0f };
        const bg2e::math::float3 eye = { 0.0f, 0.0f, -5.0f };
        const bg2e::math::float3 up = { 0.0f, 1.0f, 0.0f };

        // Set view and projection matrix for view 0.
        {
            bg2e::math::float4x4 view = bg2e::math::float4x4::Identity();
            view.lookAt(eye, at, up);

            auto aspectRatio = static_cast<float>(window()->width()) / static_cast<float>(window()->height());
            
            bg2e::math::float4x4 proj = bg2e::math::float4x4::Perspective(60.0f, aspectRatio, 0.1f, 100.0f);
            bgfx::setViewTransform(0, view.raw(), proj.raw());
        }
        
        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303960ff, 1.0f, 0.0f);
        
        bgfx::touch(window()->viewId());
        uint64_t state = 0
            | BGFX_STATE_WRITE_R
            | BGFX_STATE_WRITE_G
            | BGFX_STATE_WRITE_B
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CCW
            | BGFX_STATE_MSAA
            | UINT64_C(0)// triangle list     BGFX_STATE_PT_TRISTRIP
            ;
        
        static float elapsed = 0;
        elapsed += (delta / 1000.0f);
        bg2e::math::float4x4 mtx = bg2e::math::float4x4::Identity();
		mtx.rotate(elapsed, 1.0f, 0.0f, 0.0f)
			.rotate(elapsed * 2.0f, 0.0f, 1.0f, 0.0f);
        
		bg2e::math::float4x4 normMatrix = mtx;
		normMatrix.invert().transpose();

        bgfx::setTransform(mtx.raw());
		bgfx::setUniform(_lightPositionHandle, &bg2e::math::float4(2.0f, 2.0f, -5.0f,0.0f));
		bgfx::setUniform(_normalMatHandle, normMatrix.raw());
        bgfx::setVertexBuffer(window()->viewId(), _vertexBuffer);
        bgfx::setIndexBuffer(_indexBuffer);
        
        bgfx::setState(state);
        
        bgfx::submit(window()->viewId(), _program);
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
        
        bgfx::frame();
    }
    
    void destroy() {
        std::cout << "Destroy" << std::endl;
        
        bgfx::destroy(_indexBuffer);
        bgfx::destroy(_vertexBuffer);
        bgfx::destroy(_program);
		bgfx::destroy(_lightPositionHandle);
		bgfx::destroy(_normalMatHandle);
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
	bgfx::UniformHandle _lightPositionHandle;
	bgfx::UniformHandle _normalMatHandle;
    
    bx::DefaultAllocator _allocator;
    bx::FileReaderI * _fileReader;
};

const bgfx::EmbeddedShader MyEventHandler::_shaders[] = {
   BGFX_EMBEDDED_SHADER(shaders::basic_vertex),
   BGFX_EMBEDDED_SHADER(shaders::basic_fragment),
   BGFX_EMBEDDED_SHADER(shaders::phong_vertex),
   BGFX_EMBEDDED_SHADER(shaders::phong_fragment),
   
   BGFX_EMBEDDED_SHADER_END()
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
 
