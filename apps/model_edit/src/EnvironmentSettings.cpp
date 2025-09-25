//
//  EnvironmentSettings.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "EnvironmentSettings.hpp"

#include "AppDelegate.hpp"

EnvironmentSettings::EnvironmentSettings(AppDelegate* delegate)
	: ToolWindow(Right, delegate)
{

}

void EnvironmentSettings::init(uint32_t uiWidth, uint32_t uiHeight)
{
	ToolWindow::init(uiWidth, uiHeight);    
    _window.setTitle("Environment Options");
}

void EnvironmentSettings::draw(bg2e::render::RendererBasicForward * renderer, bg2e::scene::EnvironmentComponent * environment)
{
    auto drawSkybox = renderer->drawSkybox();
    _window.draw([&]() {
        bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
                
        if (environment)
        {
            auto assetPath = bg2e::base::PlatformTools::assetPath();
            if (bg2e::ui::BasicWidgets::button("Mirrored Hall"))
            {
                environment->setEnvironmentImage(assetPath, "mirrored_hall_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Theater"))
            {
                environment->setEnvironmentImage(assetPath, "theater_01_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Autum Field"))
            {
                environment->setEnvironmentImage(assetPath, "autumn_field_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Gothic Manor"))
            {
                environment->setEnvironmentImage(assetPath, "gothic_manor_01_4k.hdr");
            }
        }
    });
    
    renderer->setDrawSkybox(drawSkybox);
}
