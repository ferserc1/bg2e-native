//
//  ToolBar.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>

#include "EnvironmentSettings.hpp"
#include "SubmeshWindow.hpp"

class  AppDelegate;

class ToolBar {
public:
    ToolBar(AppDelegate * delegate) :_appDelegate{ delegate } {}

    void init(
        uint32_t uiWidth,
        std::shared_ptr<EnvironmentSettings> envSettings,
        std::shared_ptr<SubmeshWindow> submeshWindow
    );
    
    void resize(uint32_t width);
    
    void draw();
    
protected:

    AppDelegate * _appDelegate;
    
    bg2e::ui::Window _window;
    
    std::shared_ptr<EnvironmentSettings> _environmentSettings;
	std::shared_ptr<SubmeshWindow> _submeshWindow;
    
    uint32_t _height = 40;
};
