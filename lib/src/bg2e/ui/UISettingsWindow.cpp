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

#include <bg2e/ui/UISettingsWindow.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>

#include <string>

namespace bg2e::ui {

void UISettingsWindow::init()
{
    setTitle("UI Settings");
    setSize(320, 350);
    close();

    setDrawFunction([this]() {
        drawUI();
    });
}

void UISettingsWindow::drawUI()
{
    float scale = UserInterface::getScale();
    if (Input::sliderFloat("Interface Scale", &scale, 1.0f, 2.0f))
    {
        UserInterface::setScale(scale);
    }

    BasicWidgets::separator("Gizmos");

    using GizmoType = bg2e::manipulation::GizmoType;
    using GizmoComponent = bg2e::manipulation::GizmoComponent;

    struct GizmoTypeEntry {
        const char * label;
        GizmoType type;
    };
    static const GizmoTypeEntry types[] = {
        { "Camera",            GizmoType::Camera },
        { "Point Light",       GizmoType::PointLight },
        { "Spot Light",        GizmoType::SpotLight },
        { "Directional Light", GizmoType::DirectionalLight },
        { "Environment",       GizmoType::Environment },
    };

    for (auto& entry : types)
    {
        if (BasicWidgets::collapsingHeader(entry.label))
        {
            bool visible = GizmoComponent::isGizmoVisible(entry.type);
            if (BasicWidgets::checkBox(std::string("Visible##") + entry.label, &visible))
            {
                GizmoComponent::setGizmoVisible(entry.type, visible);
            }

            float opacity = GizmoComponent::gizmoOpacity(entry.type);
            if (Input::sliderFloat(std::string("Opacity##") + entry.label, &opacity, 0.0f, 1.0f))
            {
                GizmoComponent::setGizmoOpacity(entry.type, opacity);
            }

            float gizmoScale = GizmoComponent::gizmoScale(entry.type);
            if (Input::sliderFloat(std::string("Scale##") + entry.label, &gizmoScale, 0.01f, 0.5f))
            {
                GizmoComponent::setGizmoScale(entry.type, gizmoScale);
            }
        }
    }
}

}
