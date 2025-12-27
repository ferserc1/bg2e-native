
#include <bg2e/ui/Window.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

const char * Window::s_defaultWindowTitle = "Window";
uint32_t Window::s_windowIndex = 0;

void Window::draw(
    std::function<void()> drawFunction,
    std::function<void()> menuFunction
) {
	if (!_open)
	{
		return;
	}

    // The default window title is Window, but there cannot be more than one window with the same title
    if (_title == s_defaultWindowTitle)
    {
        _title = std::string(s_defaultWindowTitle) + "##" + std::to_string(++s_windowIndex);
    }

    updateFlags();
    bool* open = options.noClose ? nullptr : &_open;
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(float(options.minWidth), float(options.minHeight)),
        ImVec2(float(options.maxWidth), float(options.maxHeight))
    );
    if (_posX >= 0 && _posY >= 0)
    {
        ImGui::SetNextWindowPos(
            ImVec2(float(_posX), float(_posY)),
            options.noMove ? 0 : ImGuiCond_FirstUseEver
        );
    }
    if (_width>0 && _height > 0)
    {
        ImGui::SetNextWindowSize(
            ImVec2(float(_width), float(_height)),
            options.noResize ? 0 : ImGuiCond_FirstUseEver
        );
    }

    if (ImGui::Begin(_title.c_str(), open, _windowFlags))
    {
        // Draw menu
        if (menuFunction || _menuFunction)
        {
            if (ImGui::BeginMenuBar())
            {
                if (_menuFunction)
                {
                    _menuFunction();
                }
                if (menuFunction)
                {
                    menuFunction();
                }
                ImGui::EndMenuBar();
            }
        }
        drawFunction();
    }
    ImGui::End();
}

void Window::draw()
{
    draw(_drawFunction);
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
