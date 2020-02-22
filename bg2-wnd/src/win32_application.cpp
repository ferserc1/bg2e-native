
#include <bg2wnd/win32_application.hpp>
#include <bg2wnd/win32_window.hpp>

#if BG2_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace bg2wnd {

#if BG2_PLATFORM_WINDOWS

	void Win32Application::build() {
		
	}

	int Win32Application::run() {
		MSG msg;
		int exitCode = 0;
		while (_windows.size() > 0) {
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					_windows.clear();
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				std::vector<std::shared_ptr<Window>>::iterator closingWindow = _windows.end(), w;
				for (w = _windows.begin(); w != _windows.end(); ++w) {
					if ((*w)->shouldClose()) {
						closingWindow = w;
						break;
					}
					else if ((*w)->windowDelegate()) {		
						(*w)->setT1Clock(std::clock());
						(*w)->windowDelegate()->update((*w)->deltaTime());
						(*w)->windowDelegate()->draw();						
						(*w)->setT0Clock(std::clock());
					}
				}

				if (closingWindow != _windows.end()) {
					(*closingWindow)->destroy();
					_windows.erase(closingWindow);
				}
			}
		}

		Destroy();

		return exitCode;
	}

	Window * Win32Application::getWindow(bg2base::plain_ptr nativeWindowHandler) {
		for (auto w : _windows) {
			Win32Window * window = dynamic_cast<Win32Window*>(w.get());
			if (window && window->hWnd() == nativeWindowHandler) {
				return w.get();
			}
		}
		return nullptr;
	}

#else
	void Win32Application::build() {}
	int Win32Application::run() {}
	Window * Win32Application::getWindow(bg2base::plain_ptr nativeWindowHandler) {}
#endif

}