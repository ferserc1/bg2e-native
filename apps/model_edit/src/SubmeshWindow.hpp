
#pragma once

#include <bg2e.hpp>

class AppDelegate;
class SubmeshWindow : public bg2e::ui::Window {
public:
	void init(AppDelegate * delegate);

    void cleanup();
    
private:
	void drawSubmeshEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex);
	void drawMaterialEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex);
 
    AppDelegate * _appDelegate;
    
    bg2e::ui::MaterialEditor _materialEditor;
};
