#pragma once

#include <iostream>

#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

class Log {
public:
    enum class Level {
        INFO = 0,
        WARNING = 1,
        ERROR = 2,
        DEBUG = 3
    };

    static bool isDebug();

    Log(Level l) : _level{l} {}

    template <typename T>
    Log& operator << (const T & value) {
        if ((_level == Level::DEBUG && isDebug()) || _level != Level::DEBUG) {
            std::cout << value;
        }
        return *this;
    }

protected:
    Level _level;
};

}
}

#define bg2e_log_info bg2e::base::Log(bg2e::base::Log::Level::INFO)
#define bg2e_log_warning bg2e::base::Log(bg2e::base::Log::Level::WARNING)
#define bg2e_log_error bg2e::base::Log(bg2e::base::Log::Level::ERROR)
#define bg2e_log_debug bg2e::base::Log(bg2e::base::Log::Level::DEBUG)
#define bg2e_log_end "\n"
