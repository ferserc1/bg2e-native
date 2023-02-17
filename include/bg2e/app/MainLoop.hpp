
#ifndef bg2e_app_index_hpp
#define bg2e_app_index_hpp

#include <bg2e/app/Window.hpp>
#include <memory>

namespace bg2e {
namespace app {

class MainLoop {
public:
    MainLoop(Window * wnd);
    
    int run();
    
protected:
    std::unique_ptr<Window> _window;
};

}
}

#endif
