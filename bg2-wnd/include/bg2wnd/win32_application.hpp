#ifndef _bg2_wnd_win32_application_hpp_
#define _bg2_wnd_win32_application_hpp_

#include <bg2wnd/application.hpp>
#include <bg2base/platform.hpp>

#include <unordered_map>
#include <memory>

namespace bg2wnd {

	class Win32Application : public Application {
	public:

		virtual void build() override;

		virtual int run() override;

		virtual Window * getWindow(bg2base::plain_ptr nativeWindowHandler) override;

	};

}

#endif