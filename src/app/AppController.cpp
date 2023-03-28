
#include <bg2e/app/AppController.hpp>
#include <bg2e/app/Window.hpp>

namespace bg2e {
namespace app {

void AppController::resize(uint32_t w, uint32_t h)
{
    window().renderer().resize(w, h);
}

void AppController::frame(float delta)
{
    window().renderer().update(delta);
}

void AppController::display()
{
    window().renderer().drawFrame();
}

void AppController::destroy()
{
    window().renderer().destroy();
}

}
}
