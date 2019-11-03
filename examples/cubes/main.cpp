
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

const bgfx::EmbeddedShader s_phongShadersBasic[] = {
   BG2E_EMBEDDED_SHADER(shaders::phong_vertex),
   BG2E_EMBEDDED_SHADER(shaders::phong_fragment),

   BG2E_EMBEDDED_SHADER_END()
};

class PhongShader : public bg2e::base::Shader {
public:
	PhongShader() {}

	virtual void bindUniforms(bg2e::base::Pipeline*, bg2e::base::PolyList* plist, bg2e::base::Material* material, const bg2e::math::float4x4& modelMatrix) {
		bg2e::math::float4x4 normalMatrix = modelMatrix;
		normalMatrix.invert().transpose();

		bgfx::setUniform(_lightPositionHandle, &bg2e::math::float4(2.0f, 2.0f, -5.0f, 0.0f));
		bgfx::setUniform(_normalMatHandle, normalMatrix.raw());
		bgfx::setTexture(0, _textureUniformHandle, material->diffuse().texture->textureHandle());
		bgfx::setTexture(1, _normalUniformHandle, material->normal().texture->textureHandle());
	}

protected:
	virtual ~PhongShader() {}

	virtual bgfx::ProgramHandle loadProgram(bgfx::RendererType::Enum type) {
		_lightPositionHandle = bgfx::createUniform("lightPosition", bgfx::UniformType::Vec4);
		_normalMatHandle = bgfx::createUniform("u_normal", bgfx::UniformType::Mat4);
		_textureUniformHandle = bgfx::createUniform("s_diffuseTexture", bgfx::UniformType::Sampler);
		_normalUniformHandle = bgfx::createUniform("s_normalTexture", bgfx::UniformType::Sampler);

		bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(s_phongShadersBasic, type, "shaders::phong_vertex");
		bgfx::ShaderHandle fsh = bgfx::createEmbeddedShader(s_phongShadersBasic, type, "shaders::phong_fragment");

		return bgfx::createProgram(vsh, fsh, true);
	}

	bgfx::UniformHandle _lightPositionHandle = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle _normalMatHandle = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle _textureUniformHandle = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle _normalUniformHandle = BGFX_INVALID_HANDLE;
};

class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
        
    void init() {
        bg2e::base::MeshData meshData;
        bg2e::utils::generateCube(2.0f, meshData);
    
        _plist = new bg2e::base::PolyList();
        _plist->build(meshData);
         
       	bg2e::base::path dataPath("data");
		auto diffuse = bg2e::db::loadTexture(dataPath.pathAddingComponent("texture.jpg"));
		auto normal = bg2e::db::loadTexture(dataPath.pathAddingComponent("texture_nm.jpg"));

		_material = new bg2e::base::Material();
		_material->setDiffuse(diffuse);
		_material->setNormal(normal);

		_pipeline = new bg2e::base::Pipeline(window()->viewId());
		_pipeline->setShader(new PhongShader());
		_pipeline->setClearColor(bg2e::math::color(0x51B868FF));
    }
        
    void resize(uint32_t w, uint32_t h) {
        bgfx::setViewRect(window()->viewId(), 0, 0, window()->width(), window()->height());
    }
    
    void update(float delta) {        
        const bg2e::math::float3 at = { 0.0f, 0.0f, 0.0f };
        const bg2e::math::float3 eye = { 0.0f, 0.0f, -5.0f };
        const bg2e::math::float3 up = { 0.0f, 1.0f, 0.0f };
        auto aspectRatio = static_cast<float>(window()->width()) / static_cast<float>(window()->height());
		_pipeline->view()
			.identity()
			.lookAt(eye, at, up);
		_pipeline->projection()
			.perspective(60.0f, aspectRatio, 0.1f, 100.0f);
        
        
        static float elapsed = 0;
        elapsed += (delta / 1000.0f);
        bg2e::math::float4x4 mtx = bg2e::math::float4x4::Identity();
		mtx.rotate(elapsed, 1.0f, 0.0f, 0.0f)
			.rotate(elapsed * 2.0f, 0.0f, 1.0f, 0.0f);

		_pipeline->beginDraw();

		_pipeline->draw(_plist.getPtr(), _material.getPtr(), mtx);
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
		_plist = nullptr;
		_material = nullptr;
		_plist = nullptr;
    }
    
    void keyUp(const bg2e::wnd::KeyboardEvent & evt) {
        if (evt.keyCode() == bg2e::wnd::KeyboardEvent::KeyF1) {
            _showStats = !_showStats;
        }
    }
    
protected:
    bool _showStats = false;

    bg2e::ptr<bg2e::base::PolyList> _plist;
	bg2e::ptr<bg2e::base::Material> _material;
	bg2e::ptr<bg2e::base::Pipeline> _pipeline;
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
 
