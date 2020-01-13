
#include <bg2wnd/win32_window.hpp>
#include <bg2wnd/application.hpp>
#include <iostream>

#if BG2_PLATFORM_WINDOWS
#include <windows.h>

// Header and library for high dpi support
#include <ShellScalingApi.h>
#pragma comment(lib, "shcore.lib")

#include <stdexcept>

static std::string WindowClass = "Bg2NativeWindow";


#endif

namespace bg2wnd {

#if BG2_PLATFORM_WINDOWS

	Win32Window::~Win32Window() {
		std::cout << "Win32Window destructor called" << std::endl;
	}

	void Win32Window::build() {
		WNDCLASSA wc;
		DWORD dwExStyle;
		DWORD dwStyle;
		RECT rect;

		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

		if (_size.width() == 0) {
			_size.setWidth(300);
		}
		if (_size.height() == 0) {
			_size.setHeight(300);
		}

		rect.left = _position.x();
		rect.right = static_cast<long>(_size.width() + _position.x());
		rect.top = _position.y();
		rect.bottom = static_cast<long>(_size.height() + _position.y());

		static int32_t classIndex = 0;
		_hInstance = GetModuleHandle(0);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC) Win32Window::WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = bg2base::native_cast<HINSTANCE>(_hInstance);
		wc.hIcon = LoadIcon(0, IDI_WINLOGO);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = 0;
		wc.lpszMenuName = 0;
		_windowClass = WindowClass + std::to_string(classIndex);
		wc.lpszClassName = _windowClass.c_str();
		classIndex++;

		if (!RegisterClassA(&wc)) {
			throw std::runtime_error("Unexpected error registering window class.");
		}
//
//		if (_fullscreen) {
//			DEVMODE dmScreenSettings;
//			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
//			dmScreenSettings.dmSize = sizeof(dmScreenSettings);
//			dmScreenSettings.dmPelsWidth = _rect.width();
//			dmScreenSettings.dmPelsHeight = _rect.height();
//			dmScreenSettings.dmBitsPerPel = 32;
//			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
//
//			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
//				_fullscreen = false;
//			}
//		}
//
//		if (_fullscreen) {
//			dwExStyle = WS_EX_APPWINDOW;
//			dwStyle = WS_POPUP;
//			_rect.x(0);
//			_rect.y(0);
//			rect.left = 0;
//			rect.right = static_cast<long>(_rect.width());
//			rect.top = 0;
//			rect.bottom = static_cast<long>(_rect.height());
//		}
//		else {
			dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
			dwStyle = WS_OVERLAPPEDWINDOW;
//		}
//
		bool haveMenu = false;
		AdjustWindowRectEx(&rect, dwStyle, haveMenu, dwExStyle);
		int newPosX = _position.x() + _position.x() - rect.left;
		int newPosY = _position.y() + _position.y() - rect.top;
//
		LPCSTR title = _title.c_str();
		if (!(_hWnd = CreateWindowExA(dwExStyle, _windowClass.c_str(), title, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			_position.x(), _position.y(), rect.right - rect.left, rect.bottom - rect.top,
			0, 0, bg2base::native_cast<HINSTANCE>(_hInstance), 0)))
		{
			destroy();
			throw std::runtime_error("Unexpected error creating window");
		}

		ShowWindow(bg2base::native_cast<HWND>(_hWnd), SW_SHOW);
		SetForegroundWindow(bg2base::native_cast<HWND>(_hWnd));
		SetFocus(bg2base::native_cast<HWND>(_hWnd));


		_position.setX(newPosX);
		_position.setY(newPosY);

		if (windowDelegate()) {
			windowDelegate()->init();
			windowDelegate()->resize(_size);
		}
//
//		if (!_iconPath.empty()) {
//			setIcon(_iconPath);
//		}
	}

	bool Win32Window::shouldClose() {
		return _shouldClose;
	}

	void Win32Window::destroy() {
		//_controller->destroy();
		if (windowDelegate()) {
			windowDelegate()->cleanup();
		}

		//if (_fullscreen) {
		//	ChangeDisplaySettings(0, 0);
		//}

		if (_hDC) {
			ReleaseDC(bg2base::native_cast<HWND>(_hWnd), bg2base::native_cast<HDC>(_hDC));
			_hDC = 0;
		}

		if (_hWnd) {
			DestroyWindow(bg2base::native_cast<HWND>(_hWnd));
			_hWnd = 0;
		}

		UnregisterClassA(_windowClass.c_str(), bg2base::native_cast<HINSTANCE>(_hInstance));
	}

	LRESULT CALLBACK Win32Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		Win32Window * window = Application::Get()->getWindow<Win32Window>(hWnd);
		WindowDelegate * winDelegate = window != nullptr ? window->windowDelegate() : nullptr;
		KeyboardEvent kbEvent(KeyCode::KeyUNKNOWN, false, false, false, false);
		MouseEvent mouseEvent(0,0,false, false, false, false, false, 0.0f, 0.0f);

		if (window && winDelegate) {
			switch (uMsg) {
			case WM_CLOSE:
				window->_shouldClose = true;
				break;
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				if (wParam == VK_MENU) {
					//altPressed = true;
				}
//				fillKeyboard(kbEvent.keyboard(), static_cast<unsigned char>(wParam), altPressed);

				winDelegate->keyDown(kbEvent);
				break;
			case WM_CHAR:
//				fillKeyboard(kbEvent.keyboard(), static_cast<unsigned long>(wParam), false);
				winDelegate->charPress(kbEvent);
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if (wParam == VK_MENU) {
//					altPressed = false;
				}
//				fillKeyboard(kbEvent.keyboard(), static_cast<unsigned char>(wParam), altPressed);
				winDelegate->keyUp(kbEvent);
				break;
			case WM_LBUTTONDOWN:
//				mainLoop->mouse().setMouseDown(bg::base::Mouse::kLeftButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseDown(mouseEvent);
				break;
			case WM_LBUTTONUP:
//				mainLoop->mouse().setMouseUp(bg::base::Mouse::kLeftButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				mouseEvent.mouse().setReleasedButton(bg::base::Mouse::kLeftButton);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseUp(mouseEvent);
				break;
			case WM_RBUTTONDOWN:
//				mainLoop->mouse().setMouseDown(bg::base::Mouse::kRightButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseDown(mouseEvent);
				break;
			case WM_RBUTTONUP:
//				mainLoop->mouse().setMouseUp(bg::base::Mouse::kRightButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				mouseEvent.mouse().setReleasedButton(bg::base::Mouse::kRightButton);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseUp(mouseEvent);
				break;
			case WM_MBUTTONDOWN:
//				mainLoop->mouse().setMouseDown(bg::base::Mouse::kMiddleButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseDown(mouseEvent);
				break;
			case WM_MBUTTONUP:
//				mainLoop->mouse().setMouseUp(bg::base::Mouse::kMiddleButton);
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				mouseEvent.mouse().setReleasedButton(bg::base::Mouse::kMiddleButton);
//				fillMouseEvent(mouseEvent, mainLoop);
				winDelegate->mouseUp(mouseEvent);
				break;
			case WM_MOUSEMOVE:
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				fillMouseEvent(mouseEvent, mainLoop);
//				if (mouseEvent.mouse().anyButtonPressed()) {
//					controller->mouseDrag(mouseEvent);
//				}
				winDelegate->mouseMove(mouseEvent);
				break;
			case WM_MOUSEWHEEL:
//				fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//				fillMouseEvent(mouseEvent, mainLoop);
//				if ((short) HIWORD(wParam) > 0) {
//					mouseEvent.setDelta(bg::math::Vector2(0.0f, -1.0f));
//				}
//				else {
//					mouseEvent.setDelta(bg::math::Vector2(0.0f, 1.0f));
//				}
				winDelegate->mouseWheel(mouseEvent);
				break;
			case WM_MOUSELEAVE:
//				if (mainLoop->mouse().getButtonStatus(bg::base::Mouse::kLeftButton)) {
//					mainLoop->mouse().setMouseUp(bg::base::Mouse::kLeftButton);
//					fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//					fillMouseEvent(mouseEvent, mainLoop);
//					controller->mouseUp(mouseEvent);
//				}
//
//				if (mainLoop->mouse().getButtonStatus(bg::base::Mouse::kRightButton)) {
//					mainLoop->mouse().setMouseUp(bg::base::Mouse::kRightButton);
//					fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//					fillMouseEvent(mouseEvent, mainLoop);
//					controller->mouseUp(mouseEvent);
//				}
//
//				if (mainLoop->mouse().getButtonStatus(bg::base::Mouse::kMiddleButton)) {
//					mainLoop->mouse().setMouseUp(bg::base::Mouse::kMiddleButton);
//					fillKeyboard(mouseEvent.keyboard(), '\0', altPressed);
//					fillMouseEvent(mouseEvent, mainLoop);
//					controller->mouseUp(mouseEvent);
//				}
				break;
			case WM_SIZE:
			{
				int w = static_cast<int>(LOWORD(lParam));
				int h = static_cast<int>(HIWORD(lParam));
				window->setSize({ w, h });
				winDelegate->resize({ w, h });
				break;
			}
			case WM_MOVE:
			{
				int x = static_cast<int>(LOWORD(lParam));
				int y = static_cast<int>(HIWORD(lParam));
				window->setPosition({ x, y });
				break;
			}
			}
		}

//			switch (uMsg) {
//			case WM_DPICHANGED:
//				std::cout << "DPI changed" << std::endl;
//				break;
//			case WM_ACTIVATE:
//				break;
//			case WM_SYSCOMMAND:
//				if ((wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) &&
//					bg::system::EnergySaver::Get().screenIdleTimerDisabled())
//				{
//					return 0;
//				}
//				break;
//			case WM_COMMAND: {
//				bg::wnd::MenuItemIdentifier cmd = static_cast<bg::wnd::MenuItemIdentifier>(LOWORD(wParam));
//
//				bool found = false;
//				if (controller->eventHandler()) {
//					for (auto item : window->menu()) {
//						item->someMenuItem([&](auto item, auto index) -> bool {
//							if (item.identifier == cmd) {
//								found = true;
//								controller->eventHandler()->menuSelected(item.title, item.identifier);
//							}
//							return found;
//							});
//					}
//				}
//				if (!found) {
//					bg::wnd::Win32PopUpMenu::ProcessCommand(cmd);
//				}
//			}
//						   break;

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

#else
	Win32Window::~Win32Window() {}
	void Win32Window::build() {}
	bool Win32Window::shouldClose() { return true; }
#endif

}
