#ifndef _bg2_wnd_win32_application_hpp_
#define _bg2_wnd_win32_application_hpp_

#include <bg2wnd/application.hpp>

namespace bg2wnd {

	class Win32Application : public Application {
	public:

		virtual void build() override;

		virtual int run() override;

	};

}

#endif