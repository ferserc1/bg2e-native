#pragma once

#include <string>

#ifdef _WIN32

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#ifdef _WINDLL
#define BG2E_API __declspec(dllexport)
#else
#define BG2E_API __declspec(dllimport)
#endif

#elif __APPLE__

#define BG2E_API

#else

#define BG2E_LINUX

#define BG2E_API

#include <cstdint>

#endif
