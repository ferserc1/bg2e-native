
#ifndef _bg2e_platform_hpp_
#define _bg2e_platform_hpp_

#include <bx/bx.h>

#define BG2E_PLATFORM_ANDROID           0
#define BG2E_PLATFORM_BSD               0
#define BG2E_PLATFORM_EMSCRIPTEN        0
#define BG2E_PLATFORM_IOS               0
#define BG2E_PLATFORM_LINUX             0
#define BG2E_PLATFORM_OSX               0
#define BG2E_PLATFORM_RPI               0
#define BG2E_PLATFORM_STEAMLINK         0
#define BG2E_PLATFORM_WINDOWS           0

#if BX_PLATFORM_ANDROID
    #undef BG2E_PLATFORM_ANDROID
    #define BG2E_PLATFORM_ANDROID       1
#elif BX_PLATFORM_BSD
    #undef BG2E_PLATFORM_BSD
    #define BG2E_PLATFORM_BSD           1
#elif BX_PLATFORM_EMSCRIPTEN
    #define BG2E_PLATFORM_EMSCRIPTEN    1
#elif BX_PLATFORM_IOS
    #undef BG2E_PLATFORM_IOS
    #define BG2E_PLATFORM_IOS           1
#elif BX_PLATFORM_LINUX
    #undef BG2E_PLATFORM_LINUX
    #define BG2E_PLATFORM_LINUX         1
#elif BX_PLATFORM_OSX
    #undef BG2E_PLATFORM_OSX
    #define BG2E_PLATFORM_OSX           1
#elif BX_PLATFORM_RPI
    #undef BG2E_PLATFORM_RPI
    #define BG2E_PLATFORM_RPI           1
#elif BX_PLATFORM_STEAMLINK
    #undef BG2E_PLATFORM_STEAMLINK
    #define BG2E_PLATFORM_STEAMLINK     1
#elif BX_PLATFORM_WINDOWS
    #undef BG2E_PLATFORM_WINDOWS
    #define BG2E_PLATFORM_WINDOWS       1
#endif

#endif

