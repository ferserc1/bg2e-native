
#include <iostream>
#include <bx/bx.h>
#include <bx/file.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <bg2e/bgfx_tools.hpp>

#include <bg2e/base.hpp>
#include <bg2e/wnd.hpp>
#include <bg2e/utils.hpp>
#include <bg2e/math.hpp>
#include <bg2e/db.hpp>

#if BG2E_PLATFORM_WINDOWS==1
#include "win64/example_shaders.h"
#elif BG2E_PLATFORM_OSX==1
#include "osx/example_shaders.h"
#elif BG2E_PLATFORM_LINUX==1
#include "linux/example_shaders.h"
#endif

class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
        
    void init() {
        std::cout << "Init" << std::endl;
        
        bg2e::base::MeshData meshData;
        bg2e::utils::generateCube(2.0f, meshData);
    
        _plist = new bg2e::base::PolyList();
        _plist->build(meshData);
         
        bgfx::RendererType::Enum type = bgfx::getRendererType();

		bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::phong_vertex");
		bgfx::ShaderHandle fsh = bgfx::createEmbeddedShader(_shaders, type, "shaders::phong_fragment");
        _program = bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);

		_lightPositionHandle = bgfx::createUniform("lightPosition", bgfx::UniformType::Vec4);
		_normalMatHandle = bgfx::createUniform("u_normal", bgfx::UniformType::Mat4);

		bg2e::base::path dataPath("data");
		_textureImage = bg2e::db::loadImage(dataPath.pathAddingComponent("texture.jpg"));

		const bgfx::Memory* mem = bgfx::makeRef(
			_textureImage->data(),
			_textureImage->dataSize());
		_textureHandle = bgfx::createTexture2D(
			uint16_t(_textureImage->size().width()),
			uint16_t(_textureImage->size().height()),
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			0,
			mem
		);

		if (bgfx::isValid(_textureHandle)) {
			bgfx::setName(_textureHandle, "texture.jpg");
		}

		_textureUniformHandle = bgfx::createUniform("s_diffuseTexture", bgfx::UniformType::Sampler);

		_normalImage = bg2e::db::loadImage(dataPath.pathAddingComponent("texture_nm.jpg"));

		const bgfx::Memory* mem2 = bgfx::makeRef(
			_normalImage->data(),
			_normalImage->dataSize());
		_normalHandle = bgfx::createTexture2D(
			uint16_t(_normalImage->size().width()),
			uint16_t(_normalImage->size().height()),
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			0,
			mem2
		);

		if (bgfx::isValid(_normalHandle)) {
			bgfx::setName(_normalHandle, "texture_nm.jpg");
		}

		_normalUniformHandle = bgfx::createUniform("s_normalTexture", bgfx::UniformType::Sampler);
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
		bgfx::setTexture(0, _textureUniformHandle, _textureHandle);
		bgfx::setTexture(1, _normalUniformHandle, _normalHandle);
        bgfx::setVertexBuffer(window()->viewId(), _plist->vertexBuffer());
        bgfx::setIndexBuffer(_plist->indexBuffer());
        
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
        
        bgfx::destroy(_program);
		bgfx::destroy(_lightPositionHandle);
		bgfx::destroy(_normalMatHandle);
		bgfx::destroy(_textureHandle);
		bgfx::destroy(_normalUniformHandle);
		bgfx::destroy(_normalHandle);
		if (_textureImage) delete _textureImage;
		if (_normalImage) delete _normalImage;
    }
    
    void keyUp(const bg2e::wnd::KeyboardEvent & evt) {
        if (evt.keyCode() == bg2e::wnd::KeyboardEvent::KeyF1) {
            _showStats = !_showStats;
        }
        std::cout << "Key up" << std::endl;
    }
    
protected:
    bool _showStats = false;
    
    bgfx::ProgramHandle _program;
	bgfx::UniformHandle _lightPositionHandle;
	bgfx::UniformHandle _normalMatHandle;
	bgfx::UniformHandle _textureUniformHandle;
	bgfx::TextureHandle _textureHandle;
	bgfx::UniformHandle _normalUniformHandle;
	bgfx::TextureHandle _normalHandle;

	bg2e::base::image* _textureImage = nullptr;
	bg2e::base::image* _normalImage = nullptr;
    
    bg2e::base::PolyList * _plist = nullptr;
    
    bx::DefaultAllocator _allocator;
    bx::FileReaderI * _fileReader;
};

const bgfx::EmbeddedShader MyEventHandler::_shaders[] = {
   BG2E_EMBEDDED_SHADER(shaders::basic_vertex),
   BG2E_EMBEDDED_SHADER(shaders::basic_fragment),
   BG2E_EMBEDDED_SHADER(shaders::phong_vertex),
   BG2E_EMBEDDED_SHADER(shaders::phong_fragment),
   
   BG2E_EMBEDDED_SHADER_END()
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
 
