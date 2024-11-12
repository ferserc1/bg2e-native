#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <bg2e/ui/UserInterface.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void UserInterfaceDelegate::drawUI()
{
	ImGui::ShowDemoWindow();
}

}
}
