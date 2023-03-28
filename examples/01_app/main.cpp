#include <bg2e/app/Window.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/AppController.hpp>

#include <iostream>
#include <iomanip>
#include <thread>
#include <math.h>

class MyAppController : public bg2e::app::AppController
{
public:
    void init()
    {
        window().renderer().setDrawFunction([&](bg2e::render::CommandQueue* queue) {
            static int frameNumber = 0;
            ++frameNumber;
            float r = (sin(frameNumber / 120.f) + 1.0) / 2.0f;
            float g = (sin(frameNumber / 120.f + 3.141592653589793) + 1.0f) / 2.0f;
            float b = (sin(frameNumber / 120.f + 1.570796326794897) + 1.0f) / 2.0f;
            queue->beginFrame(glm::vec4(r, g, b, 1.0f));
            
            bg2e::Size winSize = window().size();
            queue->setViewport(bg2e::Viewport{0, 0, winSize.width, winSize.height });
            queue->setScissor(0, 0, winSize);
            
            queue->endFrame();
        });
    }
    
    void frame(float delta)
    {
        bg2e::app::AppController::frame(delta);

        static uint32_t frames = 0;
        static float elapsed = 0.0f;
        
        if (elapsed >= 1.0f)
        {
            window().setTitle("Hello World! - " + std::to_string(frames) +  " fps");
            frames = 0;
            elapsed = 0.0f;
        }
        else
        {
            ++frames;
            elapsed += delta;
        }
    }
};

int main(int argc, char ** argv)
{
    using namespace bg2e::app;
    
    auto wnd = std::make_unique<Window>("Hello World!", 1024, 768);
    
    wnd->setAppController(std::make_unique<MyAppController>());

    MainLoop mainLoop(std::move(wnd));
    return mainLoop.run();
}
