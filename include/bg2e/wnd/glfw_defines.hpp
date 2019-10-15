
#ifndef _bg2e_glfw_defines_hpp_
#define _bg2e_glfw_defines_hpp_

#include <bg2e/platform.hpp>

#include <GLFW/glfw3.h>

#if BG2E_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BG2E_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BG2E_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3native.h>

#endif
