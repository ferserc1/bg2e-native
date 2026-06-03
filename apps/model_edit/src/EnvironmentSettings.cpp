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
    bg2e::render::Renderer * renderer
) {
    _appDelegate = delegate;
    setTitle("Environment Options");

    _lightEditor = std::make_shared<bg2e::ui::LightEditor>();
    _polarEditor = std::make_shared<bg2e::ui::PolarTransformControllerEditor>();
    _cameraSettings = std::make_shared<bg2e::ui::CameraSettings>();
    
    setDrawFunction([&, renderer]() {
        auto environment = _appDelegate->stage()->environment();
        auto cameraComp = _appDelegate->stage()->cameraComponent();
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
            if (bg2e::ui::BasicWidgets::button("Space"))
            {
                environment->setEnvironmentImage(assetPath, "HDR_multi_nebulae_1.hdr");
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
        bg2e::ui::BasicWidgets::spacing();


        bg2e::ui::BasicWidgets::separator("Camera");
        _cameraSettings->setCameraComponent(cameraComp);
        _cameraSettings->draw();

        bg2e::ui::BasicWidgets::spacing();
        bg2e::ui::BasicWidgets::separator("Lighting");
        if (bg2e::ui::BasicWidgets::button("Add Light")) {
            _appDelegate->stage()->addLight();
        }

        auto numLights = _appDelegate->stage()->lights().size();
        if (numLights > 0)
        {
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

            // Ensure that if _selectedLightIndex is valid, the light is visible in the component
           if (!_selectedLight)
           {
               _selectedLight = _appDelegate->stage()->lights()[_selectedLightIndex];
           }

            if (bg2e::ui::BasicWidgets::collapsingHeader(lightNames[_selectedLightIndex]))
            {
                bg2e::base::Light::LightType type = bg2e::base::Light::TypeDisabled;
                _lightEditor->setLightComponent(_selectedLight);
                type = _selectedLight->light().type();
                if (_lightEditor->draw())
                {
                    auto lightDrwComp = _selectedLight->ownerNode()->drawable();
                    auto lightDrw = lightDrwComp != nullptr ? lightDrwComp->drawable() : nullptr;
                    if (lightDrw)
                    {
                        lightDrw->material(0).setAlbedo(_selectedLight->light().color());
                        lightDrw->updateMaterials();
                    }
                }

                auto polarCtrl = _selectedLight->ownerNode()->getComponent<bg2e::scene::PolarTransformControllerComponent>();
                if (polarCtrl)
                {
                    _polarEditor->setComponent(polarCtrl);
                    _polarEditor->draw();

                    if (bg2e::ui::BasicWidgets::button("Remove Light"))
                    {
                        _appDelegate->stage()->removeLight(_selectedLightIndex);
                        _selectedLight.reset();
                    }
                }
            }
        }

        bg2e::ui::BasicWidgets::spacing();
        bg2e::ui::BasicWidgets::separator("Floor");
        auto floorVisible = _appDelegate->stage()->isFloorVisible();
        if (floorVisible)
        {
            if (bg2e::ui::BasicWidgets::button("Hide floor"))
            {
                _appDelegate->stage()->showFloor(false);
            }
            auto floorHeight = _appDelegate->stage()->floorHeight();
            if (bg2e::ui::Input::number("Floor Height", &floorHeight))
            {
                _appDelegate->stage()->setFlorHeight(floorHeight);
            }
        }
        else
        {
            if (bg2e::ui::BasicWidgets::button("Show floor"))
            {
                _appDelegate->stage()->showFloor(true);
            }
        }

        bg2e::ui::BasicWidgets::spacing();
        if (bg2e::ui::BasicWidgets::button("Save Settings"))
        {
            _appDelegate->stage()->saveEnvironmentSettings();
        }

        if (bg2e::ui::BasicWidgets::button("Save Settings As", true))
        {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e scene", "json, vitscnj" }
            });
            auto filePath = fd.saveFile();

            if (!filePath.empty())
            {
                _appDelegate->stage()->saveEnvironmentSettings(filePath);
            }
        }

        if (bg2e::ui::BasicWidgets::button("Restore Settings", true))
        {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e scene", "json, vitscnj" }
            });
            auto filePath = fd.openFile();

            if (!filePath.empty())
            {
                _selectedLight = nullptr;
                _selectedLightIndex = 0;
                _appDelegate->stage()->restoreEnvironmentSettings(filePath);
            }
        }

    });
    
}
