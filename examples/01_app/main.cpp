#include <bg2e/app/Window.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <iostream>

int main(int argc, char ** argv)
{
    using namespace bg2e::app;
    
    auto wnd = new Window("Hello World!", 1024, 768);
    
    MainLoop mainLoop(wnd);
    return mainLoop.run();
}
