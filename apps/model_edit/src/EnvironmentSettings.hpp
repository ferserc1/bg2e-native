
//
//  EnvironmentSettings.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>
#include "ToolWindow.hpp"

class AppDelegate;

class EnvironmentSettings : public ToolWindow {
public:
    EnvironmentSettings(AppDelegate* delegate);
    
    void init(uint32_t uiWidth, uint32_t uiHeight) override;
    
    void draw(bg2e::render::RendererBasicForward * renderer, bg2e::scene::EnvironmentComponent * environment);
};

