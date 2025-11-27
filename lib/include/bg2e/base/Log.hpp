#pragma once

#include <iostream>

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

class BG2E_API Log {
public:
    enum class Level {
        Info = 0,
        Warning = 1,
        Error = 2,
        Debug = 3
    };

    static bool isDebug();

    Log(Level l) : _level{l} {}

    template <typename T>
    Log& operator << (const T & value) {
        if ((_level == Level::Debug && isDebug()) || _level != Level::Debug) {
            std::cout << value;
        }
        return *this;
    }

protected:
    Level _level;
};

}
}

#define bg2e_log_info bg2e::base::Log(bg2e::base::Log::Level::Info)
#define bg2e_log_warning bg2e::base::Log(bg2e::base::Log::Level::Warning)
#define bg2e_log_error bg2e::base::Log(bg2e::base::Log::Level::Error)
#define bg2e_log_debug bg2e::base::Log(bg2e::base::Log::Level::Debug)
#define bg2e_log_end "\n"
