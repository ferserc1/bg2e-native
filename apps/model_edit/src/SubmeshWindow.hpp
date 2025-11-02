
#pragma once

#include <bg2e.hpp>

class AppDelegate;
class SubmeshWindow : public bg2e::ui::Window {
public:
	void init(AppDelegate * delegate);

    void cleanup();
    
private:
    AppDelegate * _appDelegate;
    
    bg2e::ui::MaterialEditor _materialEditor;
    bg2e::ui::SubmeshSelector _submeshSelector;
};
