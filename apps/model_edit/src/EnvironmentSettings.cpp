//
//  EnvironmentSettings.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "EnvironmentSettings.hpp"

#include "AppDelegate.hpp"


void EnvironmentSettings::init(
    AppDelegate * delegate,
    bg2e::render::RendererBasicForward * renderer,
    bg2e::scene::EnvironmentComponent * environment
) {
    _appDelegate = delegate;
    setTitle("Environment Options");
    
    setDrawFunction([&, renderer, environment]() {
        auto drawSkybox = renderer->drawSkybox();
        if (bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox))
        {
            renderer->setDrawSkybox(drawSkybox);
        }
                
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
    
    
}
