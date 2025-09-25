//
// ToolWindow.cpp

#include "ToolWindow.hpp"
#include "AppDelegate.hpp"

ToolWindow::ToolWindow(Alignment a, AppDelegate * appDelegate)
	: _alignment{ a }
	, _appDelegate{ appDelegate }
{
}

void ToolWindow::init(uint32_t uiWidth, uint32_t uiHeight)
{
	_viewportWidth = uiWidth;
	_viewportHeight = uiHeight;

	_window.setTitle("Tool Window");
	_window.options.noClose = true;
	_window.options.minWidth = 300;
	_window.options.minHeight = 400;
	_window.setSize(300, 400);

	initSize();
}

void ToolWindow::resizeViewport(uint32_t width, uint32_t height)
{
	_viewportWidth = width;
	_viewportHeight = height;
    initSize();
}

void ToolWindow::toggleOpen()
{
	if (_window.isOpen())
	{
		_window.close();
	}
	else
	{
		initSize();
		_window.open();
	}
}

void ToolWindow::initSize()
{
    if (_alignment == Left)
	{
        _window.setPosition(0, 50);
    }
	else
	{
		auto x = _viewportWidth - _window.width();
        _window.setPosition(x, 50);
    }
}
