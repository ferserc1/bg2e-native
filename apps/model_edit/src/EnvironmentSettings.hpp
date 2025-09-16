
//
//  EnvironmentSettings.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>

class EnvironmentSettings {
public:
    EnvironmentSettings() {}
    
    void init(uint32_t uiWidth, uint32_t uiHeight);
    
    void draw(bg2e::render::RendererBasicForward * renderer, bg2e::scene::EnvironmentComponent * environment);
    
    void resizeViewport(uint32_t width, uint32_t height);
    
    void toggleOpen();
    
protected:
    bg2e::ui::Window _window;
    uint32_t _viewportWidth;
    uint32_t _viewportHeight;
    
    void initSize();

};

