#include <bg2e/app/Window.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>

int main(int argc, char ** argv)
{
    using namespace bg2e::app;
    
    auto wnd = new Window("Hello World!", 1024, 768);
    
    glm::mat4 test = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.0f);

    
    MainLoop mainLoop(wnd);
    return mainLoop.run();
}
