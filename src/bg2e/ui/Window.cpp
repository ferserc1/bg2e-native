
#include <bg2e/ui/Window.hpp>

#include <imgui.h>

namespace bg2e {
namespace ui {

void Window::draw(std::function<void()> drawFunction)
{
	if (!_open)
	{
		return;
	}

    updateFlags();
    bool* open = options.noClose ? nullptr : &_open;
    if (ImGui::Begin(_title.c_str(), open))
    {
        drawFunction();
    }
    ImGui::End();   
}

void Window::updateFlags()
{
    _windowFlags = 0;
	if (options.noTitleBar)
    {
		_windowFlags |= ImGuiWindowFlags_NoTitleBar;
    }

	if (options.noScrollbar)
    {
		_windowFlags |= ImGuiWindowFlags_NoScrollbar;
    }

	if (!options.noMenu)
    {
		_windowFlags |= ImGuiWindowFlags_MenuBar;
    }

	if (options.noMove)
    {
		_windowFlags |= ImGuiWindowFlags_NoMove;
    }

	if (options.noResize)
    {
		_windowFlags |= ImGuiWindowFlags_NoResize;
    }

	if (options.noCollapse)
    {
		_windowFlags |= ImGuiWindowFlags_NoCollapse;
    }

	if (options.noNav)
    {
		_windowFlags |= ImGuiWindowFlags_NoNav;
    }

	if (options.noBackground)
    {
		_windowFlags |= ImGuiWindowFlags_NoBackground;
    }

	if (options.noBringToFront)
    {
		_windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
}

}
}
