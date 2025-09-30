
//
//  EnvironmentSettings.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>

class AppDelegate;

class EnvironmentSettings : public bg2e::ui::Window {
public:
    void init(
        AppDelegate * delegate,
        bg2e::render::RendererBasicForward * renderer,
        bg2e::scene::EnvironmentComponent * environment
    );
    
protected:
    AppDelegate * _appDelegate;
};

