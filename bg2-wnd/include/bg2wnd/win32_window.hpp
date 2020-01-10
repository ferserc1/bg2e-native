#ifndef _bg2_wnd_win32_application_hpp_
#define _bg2_wnd_win32_application_hpp_

#include <bg2wnd/window.hpp>
#include <bg2base/platform.hpp>

namespace bg2wnd {

	class Win32Window: public Window {
	public:

		virtual void build() override;

		virtual bool shouldClose() override;
	
	protected:
		bg2base::plain_ptr _hDC = nullptr;
		bg2base::plain_ptr _hWnd = nullptr;
		bg2base::plain_ptr _hInstance = nullptr;
	};

}

#endif
