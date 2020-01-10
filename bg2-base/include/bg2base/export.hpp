#ifndef _bg2_base_export_hpp_
#define _bg2_base_export_hpp_

#include <bg2base/platform.hpp>

#if BG2_PLATFORM_WINDOWS

    #ifdef BG2_LIBRARY
        #define EXPORT __declspec(dllexport)
    #else
        #define EXPORT __declspec(dllimport)
    #endif
#else

#define EXPORT

#endif

#endif
