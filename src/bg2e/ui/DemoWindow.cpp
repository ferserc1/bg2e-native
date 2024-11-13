
#include <bg2e/ui/DemoWindow.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void DemoWindow::draw() {
    ImGui::ShowDemoWindow();
}

}
}