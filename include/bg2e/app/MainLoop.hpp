
#ifndef bg2e_app_index_hpp
#define bg2e_app_index_hpp

#include <bg2e/export.hpp>

#include <bg2e/app/Window.hpp>
#include <memory>

namespace bg2e {
namespace app {

class BG2_EXPORT MainLoop {
public:
    MainLoop(Window * wnd);
    
    int run();
    
private:
    std::unique_ptr<Window> _window;
};

}
}

#endif
