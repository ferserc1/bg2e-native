#pragma once

#include <string>
#include <filesystem>

#include <bg2e/common.hpp>

#ifdef __APPLE__

#define BG2E_IS_MAC 1

static const bool is_mac = true;
static const bool is_windows = false;
static const bool is_linux = false;

#elif defined(_WIN32)

#define BG2E_IS_WINDOWS 1

static const bool is_mac = false;
static const bool is_windows = true;
static const bool is_linux = false;

#else

#define BG2E_IS_LINUX 1

static const bool is_mac = false;
static const bool is_windows = false;
static const bool is_linux = true;

#endif

namespace bg2e {
namespace base {

class BG2E_API PlatformTools {
public:
    static std::filesystem::path shaderPath();
    static std::filesystem::path assetPath();
};

}
}
