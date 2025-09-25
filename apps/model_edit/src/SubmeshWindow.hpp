
#pragma once

#include <bg2e.hpp>
#include "ToolWindow.hpp"

class AppDelegate;
class SubmeshWindow : public ToolWindow {
public:
	SubmeshWindow(AppDelegate * delegate);

	void init(uint32_t uiWidth, uint32_t uiHeight);
	
	void draw();

private:
	void drawSubmeshEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex);
	void drawMaterialEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex);
};
