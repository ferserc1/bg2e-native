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

class ToolBar : public bg2e::ui::Toolbar {
public:

    void init(AppDelegate * delegate);
    
protected:

    AppDelegate * _appDelegate;
};
