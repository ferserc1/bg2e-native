
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

	static std::map<WPARAM, KeyCode> s_KeyCodes = {
		{ VK_OEM_COMMA, KeyCode::KeyCOMMA },
		{ VK_OEM_MINUS, KeyCode::KeyMINUS },
		{ VK_OEM_PERIOD, KeyCode::KeyPERIOD },
		{ VK_BACK, KeyCode::KeyBACKSPACE },
		{ VK_TAB, KeyCode::KeyTAB },
		{ VK_CLEAR, KeyCode::KeyUNKNOWN },
		{ VK_RETURN, KeyCode::KeyENTER },
		{ VK_PAUSE, KeyCode::KeyPAUSE },
		{ VK_ESCAPE, KeyCode::KeyESCAPE },
		{ VK_SPACE, KeyCode::KeySPACE },
		{ VK_PRIOR, KeyCode::KeyPAGE_UP },
		{ VK_NEXT, KeyCode::KeyPAGE_DOWN },
		{ VK_END, KeyCode::KeyEND },
		{ VK_HOME, KeyCode::KeyHOME },
		{ VK_LEFT, KeyCode::KeyLEFT },
		{ VK_UP, KeyCode::KeyUP },
		{ VK_RIGHT, KeyCode::KeyRIGHT },
		{ VK_DOWN, KeyCode::KeyDOWN },
		{ VK_PRINT, KeyCode::KeyPRINT_SCREEN },
		{ VK_INSERT, KeyCode::KeyINSERT },
		{ VK_DELETE, KeyCode::KeyDELETE },
		{ VK_NUMPAD0, KeyCode::KeyNUMPAD0 },
		{ VK_NUMPAD1, KeyCode::KeyNUMPAD1 },
		{ VK_NUMPAD2, KeyCode::KeyNUMPAD2 },
		{ VK_NUMPAD3, KeyCode::KeyNUMPAD3 },
		{ VK_NUMPAD4, KeyCode::KeyNUMPAD4 },
		{ VK_NUMPAD5, KeyCode::KeyNUMPAD5 },
		{ VK_NUMPAD6, KeyCode::KeyNUMPAD6 },
		{ VK_NUMPAD7, KeyCode::KeyNUMPAD7 },
		{ VK_NUMPAD8, KeyCode::KeyNUMPAD8 },
		{ VK_NUMPAD9, KeyCode::KeyNUMPAD9 },
		{ VK_MULTIPLY, KeyCode::KeyKP_MULTIPLY },
		{ VK_ADD, KeyCode::KeyKP_ADD },
		{ VK_SUBTRACT, KeyCode::KeyKP_SUBTRACT },
		{ VK_DECIMAL, KeyCode::KeyKP_DECIMAL },
		{ VK_DIVIDE, KeyCode::KeyKP_DIVIDE },
		{ VK_F1, KeyCode::KeyF1 },
		{ VK_F2, KeyCode::KeyF2 },
		{ VK_F3, KeyCode::KeyF3 },
		{ VK_F4, KeyCode::KeyF4 },
		{ VK_F5, KeyCode::KeyF5 },
		{ VK_F6, KeyCode::KeyF6 },
		{ VK_F7, KeyCode::KeyF7 },
		{ VK_F8, KeyCode::KeyF8 },
		{ VK_F9, KeyCode::KeyF9 },
		{ VK_F10, KeyCode::KeyF10 },
		{ VK_F11, KeyCode::KeyF11 },
		{ VK_F12, KeyCode::KeyF12 },
		{ VK_F13, KeyCode::KeyF13 },
		{ VK_F14, KeyCode::KeyF14 },
		{ VK_F15, KeyCode::KeyF15 },
		{ VK_F16, KeyCode::KeyF16 },
		{ VK_F17, KeyCode::KeyF17 },
		{ VK_F18, KeyCode::KeyF18 },
		{ VK_F19, KeyCode::KeyF19 },
		{ VK_F20, KeyCode::KeyF20 },
		{ VK_F21, KeyCode::KeyF21 },
		{ VK_F22, KeyCode::KeyF22 },
		{ VK_F23, KeyCode::KeyF23 },
		{ VK_F24, KeyCode::KeyF24 },
		{ 0x30, KeyCode::Key0 },
		{ 0x31, KeyCode::Key1 },
		{ 0x32, KeyCode::Key2 },
		{ 0x33, KeyCode::Key3 },
		{ 0x34, KeyCode::Key4 },
		{ 0x35, KeyCode::Key5 },
		{ 0x36, KeyCode::Key6 },
		{ 0x37, KeyCode::Key7 },
		{ 0x38, KeyCode::Key8 },
		{ 0x39, KeyCode::Key9 },
		{ 0x41, KeyCode::KeyA },
		{ 0x42, KeyCode::KeyB },
		{ 0x43, KeyCode::KeyC },
		{ 0x44, KeyCode::KeyD },
		{ 0x45, KeyCode::KeyE },
		{ 0x46, KeyCode::KeyF },
		{ 0x47, KeyCode::KeyG },
		{ 0x48, KeyCode::KeyH },
		{ 0x49, KeyCode::KeyI },
		{ 0x4A, KeyCode::KeyJ },
		{ 0x4B, KeyCode::KeyK },
		{ 0x4C, KeyCode::KeyL },
		{ 0x4D, KeyCode::KeyM },
		{ 0x4E, KeyCode::KeyN },
		{ 0x4F, KeyCode::KeyO },
		{ 0x50, KeyCode::KeyP },
		{ 0x51, KeyCode::KeyQ },
		{ 0x52, KeyCode::KeyR },
		{ 0x53, KeyCode::KeyS },
		{ 0x54, KeyCode::KeyT },
		{ 0x55, KeyCode::KeyU },
		{ 0x56, KeyCode::KeyV },
		{ 0x57, KeyCode::KeyW },
		{ 0x58, KeyCode::KeyX },
		{ 0x59, KeyCode::KeyY },
		{ 0x5A, KeyCode::KeyZ }
	};

	static std::map<WPARAM, std::string> s_KeyCharacters = {
		{ VK_OEM_COMMA, "," },
		{ VK_OEM_MINUS, "-" },
		{ VK_OEM_PERIOD, "." },
		{ VK_TAB, "\t" },
		{ VK_RETURN, "\r" },
		{ VK_SPACE, " " },
		{ VK_NUMPAD0, "0" },
		{ VK_NUMPAD1, "1" },
		{ VK_NUMPAD2, "2" },
		{ VK_NUMPAD3, "3" },
		{ VK_NUMPAD4, "4" },
		{ VK_NUMPAD5, "5" },
		{ VK_NUMPAD6, "6" },
		{ VK_NUMPAD7, "7" },
		{ VK_NUMPAD8, "8" },
		{ VK_NUMPAD9, "9" },
		{ VK_MULTIPLY, "*" },
		{ VK_ADD, "+" },
		{ VK_SUBTRACT, "-" },
		{ VK_DECIMAL, "." },
		{ VK_DIVIDE, "/" },
		{ 0x30, "0" },
		{ 0x31, "1" },
		{ 0x32, "2" },
		{ 0x33, "3" },
		{ 0x34, "4" },
		{ 0x35, "5" },
		{ 0x36, "6" },
		{ 0x37, "7" },
		{ 0x38, "8" },
		{ 0x39, "9" },
		{ 0x41, "A" },
		{ 0x42, "B" },
		{ 0x43, "C" },
		{ 0x44, "D" },
		{ 0x45, "E" },
		{ 0x46, "F" },
		{ 0x47, "G" },
		{ 0x48, "H" },
		{ 0x49, "I" },
		{ 0x4A, "J" },
		{ 0x4B, "K" },
		{ 0x4C, "L" },
		{ 0x4D, "M" },
		{ 0x4E, "N" },
		{ 0x4F, "O" },
		{ 0x50, "P" },
		{ 0x51, "Q" },
		{ 0x52, "R" },
		{ 0x53, "S" },
		{ 0x54, "T" },
		{ 0x55, "U" },
		{ 0x56, "V" },
		{ 0x57, "W" },
		{ 0x58, "X" },
		{ 0x59, "Y" },
		{ 0x5A, "Z" }
	};

	LRESULT CALLBACK Win32Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		Win32Window * window = Application::Get()->getWindow<Win32Window>(hWnd);
		WindowDelegate * winDelegate = window != nullptr ? window->windowDelegate() : nullptr;

		auto getKeyEvent = [wParam]() -> KeyboardEvent {
			KeyCode code = KeyCode::KeyUNKNOWN;
			std::string character = "";
			if (s_KeyCodes.find(wParam) != s_KeyCodes.end()) {
				code = s_KeyCodes[wParam];
			}
			if (s_KeyCharacters.find(wParam) != s_KeyCharacters.end()) {
				character = s_KeyCharacters[wParam];
			}
			return KeyboardEvent(
				code,
				character,
				GetKeyState(VK_SHIFT) & 8000, 
				GetKeyState(VK_CONTROL) & 8000, 
				GetKeyState(VK_MENU) & 8000, 
				GetKeyState(VK_CAPITAL) & 8000);
		};

		auto getMouseEvent = [wParam, lParam, window](MouseButton evtButton = MouseButton::ButtonNone, float wheelDelta = 0.0f) -> MouseEvent {
			POINT p;
			bg2math::int2 position;
			GetCursorPos(&p);
			ScreenToClient(bg2base::native_cast<HWND>(window->hWnd()), &p);
			GetKeyState(VK_LBUTTON) & 8000;

			return MouseEvent(p.x, p.y, evtButton,
				GetKeyState(VK_LBUTTON) & 8000, 
				GetKeyState(VK_MBUTTON) & 8000, 
				GetKeyState(VK_RBUTTON) & 8000,
				false,
				false,
				0.0f, wheelDelta);
		};
		

		if (window && winDelegate) {
			switch (uMsg) {
			case WM_CLOSE:
				window->_shouldClose = true;
				break;
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				winDelegate->keyDown(getKeyEvent());
				break;
			case WM_CHAR:
				winDelegate->charPress(getKeyEvent());
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				winDelegate->keyUp(getKeyEvent());
				break;
			case WM_LBUTTONDOWN:
				winDelegate->mouseDown(getMouseEvent(MouseButton::ButtonLeft));
				break;
			case WM_LBUTTONUP:
				winDelegate->mouseUp(getMouseEvent(MouseButton::ButtonLeft));
				break;
			case WM_RBUTTONDOWN:
				winDelegate->mouseDown(getMouseEvent(MouseButton::ButtonRight));
				break;
			case WM_RBUTTONUP:
				winDelegate->mouseUp(getMouseEvent(MouseButton::ButtonRight));
				break;
			case WM_MBUTTONDOWN:
				winDelegate->mouseDown(getMouseEvent(MouseButton::ButtonMiddle));
				break;
			case WM_MBUTTONUP:
				winDelegate->mouseUp(getMouseEvent(MouseButton::ButtonMiddle));
				break;
			case WM_MOUSEMOVE:
				winDelegate->mouseMove(getMouseEvent());
				break;
			case WM_MOUSEWHEEL:
				winDelegate->mouseWheel(getMouseEvent(MouseButton::ButtonNone, static_cast<short>(HIWORD(wParam)) > 0 ? -1.0f : 1.0f));
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
