
#include <bg2wnd/win32_application.hpp>

#if BG2_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace bg2wnd {

#if BG2_PLATFORM_WINDOWS

	void Win32Application::build() {
		
	}

	int Win32Application::run() {
		return 0;
	}

#else
	void Win32Application::build() {}
	int Win32Application::run() {}
#endif

}