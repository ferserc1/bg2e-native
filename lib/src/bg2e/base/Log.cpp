
#include <bg2e/base/Log.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

bool Log::isDebug() {
#if NDEBUG
    return false;
#else
    return true;
#endif
}

}
}
