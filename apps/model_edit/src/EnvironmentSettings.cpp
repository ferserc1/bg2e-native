/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "EnvironmentSettings.hpp"

#include "AppDelegate.hpp"

#include <algorithm>


void EnvironmentSettings::init(
    AppDelegate * delegate,
    bg2e::render::RendererBasicForward * renderer,
    bg2e::scene::EnvironmentComponent * environment
) {
    _appDelegate = delegate;
    setTitle("Environment Options");

    _lightEditor = std::make_shared<bg2e::ui::LightEditor>();
    
    setDrawFunction([&, renderer, environment]() {
        auto drawSkybox = renderer->drawSkybox();
        auto skyboxBlur = renderer->skyboxBlurLevel() > 0;
        if (bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox))
        {
            renderer->setDrawSkybox(drawSkybox);
        }
        if (bg2e::ui::BasicWidgets::checkBox("Blur", &skyboxBlur))
        {
            renderer->setSkyboxBlurLevel(skyboxBlur ? 2 : 0);
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
            if (bg2e::ui::BasicWidgets::button("Black Environment"))
            {
                environment->setEnvironmentImage(assetPath, "black.jpg");
            }
            if (bg2e::ui::BasicWidgets::button("Custom Environment"))
            {
                auto path = bg2e::app::FileDialog::getOpenFilePath({
                    { "HDR Environments", "hdr" }
                });
                if (!path.empty())
                {
                    environment->setEnvironmentImage(path.string());
                }
            }
        }


        bg2e::ui::BasicWidgets::separator("Lighting");
        auto numLights = _appDelegate->stage()->lights().size();
        std::vector<std::string> lightNames(numLights);
        size_t i = 0;
        std::generate(lightNames.begin(), lightNames.end(), [&]()
        {
            return "Light " + std::to_string(i++);
        });
        if (bg2e::ui::Input::comboBox(
            "Lights in Scene",
            lightNames,
            _selectedLightIndex
        )) {
            _selectedLight = _appDelegate->stage()->lights()[_selectedLightIndex];
        }

        bg2e::base::Light::LightType type = bg2e::base::Light::TypeDisabled;
        if (_selectedLight.get() != nullptr)
        {
            _lightEditor->setLightComponent(_selectedLight);
            type = _selectedLight->light().type();
        }

        _lightEditor->draw();

        if (_selectedLight.get() != nullptr)
        {
            bg2e::ui::BasicWidgets::separator("Position and Orientation");
            auto polarCtrl = _selectedLight->ownerNode()->getComponent<bg2e::scene::PolarTransformControllerComponent>();
            if (polarCtrl)
            {
                float azimuth = polarCtrl->azimuth();
                float elevation = polarCtrl->elevation();
                float distance = polarCtrl->distance();
                if (bg2e::ui::Input::sliderFloat("Azimuth", &azimuth, 0.0f, 360.0f))
                {
                    polarCtrl->setAzimuth(azimuth);
                }
                if (bg2e::ui::Input::sliderFloat("Elevation", &elevation, -90.0f, 90.0f))
                {
                    polarCtrl->setElevation(elevation);
                }
                if (bg2e::ui::Input::sliderFloat("Distance", &distance, 0.0f, 50.0f))
                {
                    polarCtrl->setDistance(distance);
                }

                float eulerX = polarCtrl->eulerX();
                float eulerY = polarCtrl->eulerY();
                float eulerZ = polarCtrl->eulerZ();
                if (bg2e::ui::Input::sliderFloat("Euler X", &eulerX, -360.0f, 360.0f))
                {
                    polarCtrl->setEulerX(eulerX);
                }
                if (bg2e::ui::Input::sliderFloat("Euler Y", &eulerY, -360.0f, 360.0f))
                {
                    polarCtrl->setEulerY(eulerY);
                }
                if (bg2e::ui::Input::sliderFloat("Euler Z", &eulerZ, -360.0f, 360.0f))
                {
                    polarCtrl->setEulerZ(eulerZ);
                }
            }
        }





        bg2e::ui::BasicWidgets::separator("User Interface");
        if (bg2e::ui::BasicWidgets::button("Scale 2x"))
        {
            bg2e::ui::UserInterface::setScale(2.0f);
        }
        if (bg2e::ui::BasicWidgets::button("Scale 1x"))
        {
            bg2e::ui::UserInterface::setScale(1.0f);
        }
    });
    
    
}
