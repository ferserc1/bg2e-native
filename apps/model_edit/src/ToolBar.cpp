//
//  ToolBar.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "ToolBar.hpp"

void ToolBar::init(uint32_t uiWidth, std::shared_ptr<EnvironmentSettings> envSettings)
{
    _window.options.noClose = true;
    _window.options.noMove = true;
    _window.options.noTitleBar = true;
    _window.options.noMenu = true;
    _window.options.noResize = true;
    _window.options.noBringToFront = true;
    _window.setPosition(0, 0);
    _window.setSize(uiWidth, _height);
    
    _environmentSettings = envSettings;
}

void ToolBar::resize(uint32_t width)
{
    _window.setSize(width, _height);
}

void ToolBar::draw()
{
    _window.draw([&]() {
        // TODO: Build the menu bar
        using namespace bg2e::ui;
        
        if (BasicWidgets::button("Open", true))
        {
            
        }
        if (BasicWidgets::button("Save", true))
        {
        
        }
        if (BasicWidgets::button("Environment", true))
        {
            _environmentSettings->toggleOpen();
        }
        if (BasicWidgets::button("Material Editor", true))
        {
            
        }
    });
}
