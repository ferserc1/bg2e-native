
#include <iostream>

#include <bg2e/bg2e.hpp>


class MyEventHandler : public  bg2e::wnd::EventHandler {
public:
        
    void init() {
		_sceneRoot = new bg2e::scene::Node("Scene root");

        bg2e::base::MeshData meshData;
        bg2e::utils::generateCube(2.0f, meshData);
    
        auto plist = new bg2e::base::PolyList();
        plist->build(meshData);
         
       	bg2e::base::path dataPath("data");
		auto diffuse = bg2e::db::loadTexture(dataPath.pathAddingComponent("texture.jpg"));
		auto normal = bg2e::db::loadTexture(dataPath.pathAddingComponent("texture_nm.jpg"));

		auto material = new bg2e::base::Material();
		material->setDiffuse(diffuse);
		material->setNormal(normal);

		auto cube = new bg2e::scene::Node("Cube");
		auto drawable = new bg2e::scene::Drawable();
		drawable->addPolyList(plist, material);
		cube->addComponent(drawable);
		_sceneRoot->addChild(cube);

		_camera = new bg2e::scene::Camera();
		auto projection = new bg2e::base::OpticalProjectionStrategy();
		_camera->camera().setProjectionStrategy(projection);
		auto cameraNode = new bg2e::scene::Node("Camera");
		cameraNode->addComponent(_camera.getPtr());
		_sceneRoot->addChild(cameraNode);

		// TODO: Remove this
		_camera->camera().view().lookAt({ 0.0f, 0.0f, -5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

		_pipeline = new bg2e::base::Pipeline(window()->viewId());
		_pipeline->setShader(new bg2e::shaders::Phong());
		_pipeline->setClearColor(bg2e::math::color(0x51B868FF));

		_light = new bg2e::base::Light(window()->viewId());
		_light->setPosition(bg2e::math::float3(2.0f, 2.0f, 2.0f));
		_light->setDirection(bg2e::math::float3(-0.5, -0.5, -0.5));
		bg2e::base::Light::ActivateLight(_light);
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
        
		_pipeline->beginDraw();

        static float elapsed = 0;
        elapsed += (delta / 1000.0f);
		_pipeline->model().identity()
			.rotate(elapsed, 1.0f, 0.0f, 0.0f)
			.rotate(elapsed * 2.0f, 0.0f, 1.0f, 0.0f);
		//_pipeline->draw(_plist.getPtr(), _material.getPtr());
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
    }
    
    void keyUp(const bg2e::wnd::KeyboardEvent & evt) {
        if (evt.keyCode() == bg2e::wnd::KeyboardEvent::KeyF1) {
            _showStats = !_showStats;
        }
    }
    
protected:
    bool _showStats = false;

 
	bg2e::base::RenderQueue _renderQueue;
	bg2e::ptr<bg2e::base::Pipeline> _pipeline;
	bg2e::ptr<bg2e::base::Light> _light;

	bg2e::ptr<bg2e::scene::Node> _sceneRoot;
	bg2e::ptr<bg2e::scene::Camera> _camera;
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
 
