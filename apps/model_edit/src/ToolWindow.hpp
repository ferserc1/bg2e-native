
#pragma once

#include <bg2e.hpp>

class AppDelegate;

class ToolWindow {
public:
    enum Alignment {
        Left,
        Right
    };

	ToolWindow(Alignment a, AppDelegate* appDelegate);

	virtual void init(uint32_t uiWidth, uint32_t uiHeight);
	
	void resizeViewport(uint32_t width, uint32_t height);
	
	void toggleOpen();

protected:
    Alignment _alignment;
	AppDelegate* _appDelegate;
	bg2e::ui::Window _window;
	uint32_t _viewportWidth = 0;
	uint32_t _viewportHeight = 0;
	
	virtual void initSize();
};
