#include <bg2e/app/MainLoop.hpp>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>

#include <thread>

namespace bg2e {
namespace app {

int32_t MainLoop::run(app::Application * application) {
	SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags winFlags = SDL_WindowFlags(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    
    auto window = SDL_CreateWindow(
        _windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        _windowWidth,
        _windowHeight,
        winFlags
    );
    
    _vulkan.init(window);
	_userInterface.setDelegate(application->uiDelegate());
	_userInterface.init(&_vulkan);
    _renderLoop.setDelegate(application->renderDelegate());
	_renderLoop.init(&_vulkan);

	_inputManager.setDelegate(application->inputDelegate());

	_renderLoop.renderUICallback([&](VkCommandBuffer cmd, VkImageView targetImageView) {
		_userInterface.draw(cmd, targetImageView);
	});
    
    SDL_Event event;
    bool quit = false;
    bool stopRendering = false;
    
    
    // Initialize the main descriptor set allocator before executing the first frame
    _vulkan.descriptorSetAllocator().initPool();
    _renderLoop.initScene();
    while (!quit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_MINIMIZED)
            {
                stopRendering = true;
            }
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESTORED)
            {
                stopRendering = false;
            }
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                _vulkan.updateSwapchainSize();
            }

			if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
			{
				auto key = event.key.keysym.sym;
                if (event.key.state == SDL_PRESSED)
                {
					_inputManager.keyDown(KeyEvent::fromSDLEvent(event));
                }
                else if (event.key.state == SDL_RELEASED)
                {
                    _inputManager.keyUp(KeyEvent::fromSDLEvent(event));
                }
			}
            
            if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                
                if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
                {
                    auto button = event.button.button == SDL_BUTTON_LEFT ?
                        0 : event.button.button == SDL_BUTTON_MIDDLE ?
                        1 : 2;
                    if (event.button.state == SDL_PRESSED)
                    {
                        _inputManager.mouseButtonDown(button, x, y);
                    }
                    else
                    {
                        _inputManager.mouseButtonUp(button, x, y);
                    }
                }
                else
                {
                    _inputManager.mouseMove(x, y);
                }
            }
            if (event.type == SDL_MOUSEWHEEL)
            {
                _inputManager.mouseWheel(event.wheel.x, event.wheel.y);
            }
            
            _userInterface.processEvent(&event);
        }
        
        if (stopRendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        if (_vulkan.newFrame())
        {
            _renderLoop.swapchainResized();
        }
        
        _userInterface.newFrame();
        
        _renderLoop.acquireAndPresent();
    }

    _vulkan.device().waitIdle();
	_renderLoop.cleanup();
	_userInterface.cleanup();
    _vulkan.cleanup();
    SDL_DestroyWindow(window);
    
    return 0;
}

}
}
