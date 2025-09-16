//
//  ToolBar.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>

#include "EnvironmentSettings.hpp"

class ToolBar {
public:
    ToolBar() {}

    void init(uint32_t uiWidth, std::shared_ptr<EnvironmentSettings> envSettings);
    
    void resize(uint32_t width);
    
    void draw();
    
protected:
    bg2e::ui::Window _window;
    
    std::shared_ptr<EnvironmentSettings> _environmentSettings;
    
    uint32_t _height = 40;
};
