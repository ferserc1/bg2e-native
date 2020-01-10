#ifndef _bg2_base_platform_hpp_
#define _bg2_base_platform_hpp_

#define BG2_PLATFORM_WINDOWS    0
#define BG2_PLATFORM_LINUX      0
#define BG2_PLATFORM_MACOS      0

// Windows
#ifdef _WIN32
#undef BG2_PLATFORM_WINDOWS
#define BG2_PLATFORM_WINDOWS    1
#endif

// macOS
#ifdef __APPLE__
#undef BG2_PLATFORM_MACOS
#define BG2_PLATFORM_MACOS      1
#endif

// Linux
#ifdef __linux__
#undef BG2_PLATFORM_LINUX
#define BG2_PLATFORM_LINUX      1
#endif

// void pointers
namespace bg2base {

	template <class T> T native_cast(void * nativePtr) { return static_cast<T>(nativePtr); }
	typedef void * plain_ptr;

}

#endif
