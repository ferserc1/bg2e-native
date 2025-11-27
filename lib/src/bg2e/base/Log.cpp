
#include <bg2e/base/Log.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

bool Log::isDebug() {
#ifdef BG2E_IS_MAC
    #if DEBUG
        return true;
    #else
        return false;
    #endif
#elif BG2E_IS_WINDOWS
    #if _DEBUG
        return true;
    #else
        return false;
    #endif 
#else
    // TODO: Implement this for Linux
    return false;
#endif
}

}
}
