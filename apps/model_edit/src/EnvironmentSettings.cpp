//
//  EnvironmentSettings.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "EnvironmentSettings.hpp"

void EnvironmentSettings::init(uint32_t uiWidth, uint32_t uiHeight)
{
    _viewportWidth = uiWidth;
    _viewportHeight = uiHeight;
    
    _window.setTitle("Environment Options");
    _window.options.noClose = true;
    _window.options.noResize = true;
    _window.options.noMenu = true;
    
    initSize();
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

void EnvironmentSettings::resizeViewport(uint32_t width, uint32_t height)
{
    _viewportWidth = width;
    _viewportHeight = height;
}

void EnvironmentSettings::toggleOpen()
{
    if (_window.isOpen())
    {
        _window.close();
    }
    else
    {
        initSize();
        _window.open();
    }
}

void EnvironmentSettings::initSize()
{
    auto width = 300;
    auto height = 190;
    _window.setPosition(_viewportWidth - width, 40);
    _window.setSize(width, height);
}
