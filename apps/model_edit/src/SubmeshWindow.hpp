
#pragma once

#include <bg2e.hpp>

class AppDelegate;
class SubmeshWindow : public bg2e::ui::Window {
public:
	void init(AppDelegate * delegate);

    void cleanup();

    void setEditMaterial(uint32_t submeshIndex);

private:
    AppDelegate * _appDelegate;
    
    bg2e::ui::MaterialEditor _materialEditor;
    bg2e::ui::DrawableEditor _drawableEditor;
};
