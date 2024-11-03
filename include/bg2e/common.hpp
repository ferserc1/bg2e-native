#pragma once

#ifdef _WIN32

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#ifdef _WINDLL
#define BG2E_API __declspec(dllexport)
#else
#define BG2E_API __declspec(dllimport)
#endif

#else

#define BG2E_API

#endif