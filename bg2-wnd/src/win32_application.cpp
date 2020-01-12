
#include <bg2wnd/win32_application.hpp>

#if BG2_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace bg2wnd {

#if BG2_PLATFORM_WINDOWS

	void Win32Application::build() {
		
	}

	int Win32Application::run() {
		MSG msg;
		bool done = false;

		while (!done) {
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					done = true;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				//std::clock_t lastClock = controller->getLastClock();
				//std::clock_t clock = std::clock();
				//delta = static_cast<float>(clock - lastClock) / CLOCKS_PER_SEC;
				//controller->setLastClock(std::clock());
				//controller->frame(delta);
				//controller->draw();
			}
		}

		// TODO: destroy application
		// this->destroy();
		return 0;
	}

#else
	void Win32Application::build() {}
	int Win32Application::run() {}
#endif

}