#pragma once

#include <bg2e/common.hpp>

#include <functional>
#include <string>

namespace bg2e {
namespace ui {

class BG2E_API Window {
public:
	struct Options
	{
		bool noTitleBar = false;
		bool noScrollbar = false;
		bool noMenu = false;
		bool noMove = false;
		bool noResize = false;
		bool noCollapse = false;
		bool noNav = false;
		bool noBackground = false;
		bool noBringToFront = false;
		bool noClose = false;
	};

    Options options;

    inline void setTitle(const std::string & title) { _title = title; }
    inline const std::string & title() const { return _title; }

    void draw(std::function<void()> drawFunction);

	inline void close() { _open = false; }
	inline void open() { _open = true; }


protected:
    std::string _title = "Window";
    bool _open = true;

	int32_t _windowFlags = 0;

	void updateFlags();
};

}
}

