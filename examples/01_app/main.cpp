#include <bg2e/app/Window.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/AppController.hpp>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>
#include <iomanip>

class MyAppController : public bg2e::app::AppController
{
public:
    void init()
    {
        std::cout << "init" << std::endl;
    }
    
    void frame(float delta)
    {
        //std::cout << "Frame: " << std::fixed << std::setprecision(10) << delta << std::endl;
    }
    
    void display()
    {
        //std::cout << "Draw" << std::endl;
    }
    
    void destroy()
    {
        std::cout << "Destroy" << std::endl;
    }
    
    void mouseMove(const bg2e::app::MouseEvent& evt)
    {
        std::cout << "mouseMove(" << evt.mouseStatus().posX << "," << evt.mouseStatus().posY << ")"
            << " leftButton=" << (evt.mouseStatus().leftButton ? "true" : "false")
            << " middleButton=" << (evt.mouseStatus().middleButton ? "true" : "false")
            << " rightButton=" << (evt.mouseStatus().rightButton ? "true" : "false") << std::endl;
    }
    
    void mouseDrag(const bg2e::app::MouseEvent& evt)
    {
        std::cout << "mouseDrag(" << evt.mouseStatus().posX << "," << evt.mouseStatus().posY << ")"
            << " leftButton=" << (evt.mouseStatus().leftButton ? "true" : "false")
            << " middleButton=" << (evt.mouseStatus().middleButton ? "true" : "false")
            << " rightButton=" << (evt.mouseStatus().rightButton ? "true" : "false") << std::endl;
    }
    
    void mouseDown(const bg2e::app::MouseEvent& evt)
    {
        using namespace bg2e::app;
        std::cout << "mouseDown - ";
        switch (evt.button())
        {
        case MouseButton::Left:
            std::cout << "left" << std::endl;
            break;
        case MouseButton::Middle:
            std::cout << "middle" << std::endl;
            break;
        case MouseButton::Right:
            std::cout << "right" << std::endl;
            break;
        case MouseButton::None:
            break;
        }
    }
    
    void mouseUp(const bg2e::app::MouseEvent& evt)
    {
        using namespace bg2e::app;
        std::cout << "mouseUp - ";
        switch (evt.button())
        {
        case MouseButton::Left:
            std::cout << "left" << std::endl;
            break;
        case MouseButton::Middle:
            std::cout << "middle" << std::endl;
            break;
        case MouseButton::Right:
            std::cout << "right" << std::endl;
            break;
        case MouseButton::None:
            break;
        }
    }
    
    void mouseWheel(const bg2e::app::MouseEvent& evt)
    {
        std::cout << "mouseWheel - deltaX=" <<
            evt.mouseStatus().deltaX << ", deltaY=" <<
            evt.mouseStatus().deltaY << std::endl;
    }
};

int main(int argc, char ** argv)
{
    using namespace bg2e::app;
    
    auto wnd = std::make_unique<Window>("Hello World!", 1024, 768);
    
    wnd->setAppController(std::make_unique<MyAppController>());
    
    glm::mat4 test = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.0f);

    
    MainLoop mainLoop(std::move(wnd));
    return mainLoop.run();
}
