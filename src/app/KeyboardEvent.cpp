
#include <bg2e/app/KeyboardEvent.hpp>

#include <glfw/glfw3.h>

namespace bg2e {
namespace app {

std::string KeyboardEvent::keyName() const {
    const char * keyName = glfwGetKeyName(_key, 0);
    if (keyName)
    {
        return std::string(keyName);
    }
    else
    {
        return "";
    }
}

}
}
