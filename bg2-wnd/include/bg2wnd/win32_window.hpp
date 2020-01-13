#ifndef _bg2_wnd_win32_window_hpp_
#define _bg2_wnd_win32_window_hpp_

#include <bg2wnd/window.hpp>
#include <bg2base/platform.hpp>
#include <bg2base/export.hpp>

#if BG2_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace bg2wnd {

	class Win32Window: public Window {
	public:
		virtual ~Win32Window();

		virtual void build() override;

		virtual bool shouldClose() override;
	
		virtual void destroy() override;

		// Platform specific functions
		const bg2base::plain_ptr hDC() const { return _hDC; }
		const bg2base::plain_ptr hWnd() const { return _hWnd; }
		const bg2base::plain_ptr hInstance() const { return _hInstance; }

	protected:
		bg2base::plain_ptr _hDC = nullptr;
		bg2base::plain_ptr _hWnd = nullptr;
		bg2base::plain_ptr _hInstance = nullptr;
		std::string _windowClass = "";

#if BG2_PLATFORM_WINDOWS
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		bool _shouldClose = false;
#endif
	};

}

#endif
