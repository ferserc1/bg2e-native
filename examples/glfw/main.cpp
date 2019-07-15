

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>

int main(int argc, char ** argv) {
    bg2wnd::Application app;
    
    auto window = bg2wnd::Window::Create();
    app.addWindow(window);
    app.run();
    
    return 0;
}
