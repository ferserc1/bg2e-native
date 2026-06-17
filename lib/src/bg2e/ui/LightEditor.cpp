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

#include <bg2e/ui/LightEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>

namespace bg2e::ui {

LightEditor::~ LightEditor() = default;

void LightEditor::setIntensityRange(float min, float max)
{
    _intensityMin = min;
    _intensityMax = max;
}

static base::Light::LightType lightTypeByIdx(size_t idx)
{
    if (idx == 0) return base::Light::TypeOmni;
    if (idx == 1) return base::Light::TypeSpot;
    if (idx == 2) return base::Light::TypeDirectional;
    return base::Light::TypeDisabled;
}

bool LightEditor::draw()
{
    auto comp = _lightComponent.lock();
    if (!comp)
    {
        return false;
    }

    const std::vector<std::string> s_lightTypeNames = {
        "OMNI", "SPOT", "DIRECTIONAL", "DISABLED"
    };

    auto& light = const_cast<base::Light&>(comp->light());

    uint32_t currentTypeIdx = 0;
    switch (light.type())
    {
        case base::Light::TypeOmni: currentTypeIdx = 0; break;
        case base::Light::TypeSpot: currentTypeIdx = 1; break;
        case base::Light::TypeDirectional: currentTypeIdx = 2; break;
        default: currentTypeIdx = 3; break;
    }

    bool changed = false;


    BasicWidgets::separator("Basic properties");

    if (Input::comboBox(
        "Light Type",
        s_lightTypeNames,
        currentTypeIdx, false))
    {
        light.setType(lightTypeByIdx(currentTypeIdx));
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    base::Color color = light.color();
    if (Input::colorPicker("Light Color", color))
    {
        light.setColor(color);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    float intensity = light.intensity();
    if (Input::sliderFloat(
        "Light Intensity", &intensity, _intensityMin, _intensityMax))
    {
        light.setIntensity(intensity);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    BasicWidgets::separator("Shadows");
    bool castShadows = light.castShadows();
    if (BasicWidgets::checkBox("Cast Shadows", &castShadows))
    {
        light.setCastShadows(castShadows);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    bool affectReflections = light.affectsReflections();
    if (BasicWidgets::checkBox("Affect Reflections", &affectReflections))
    {
        light.setAffectsReflections(affectReflections);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }
    if (castShadows)
    {
        float sourceSize = light.sourceSize();
        if (Input::sliderFloat("Source Size", &sourceSize, 0.01f, 5.0f))
        {
            light.setSourceSize(sourceSize);
            changed = true;
            if (_onChangedFunction) _onChangedFunction();
        }

        int shadowSamples = static_cast<int>(light.shadowSamples());
        if (Input::sliderInt("Shadow Samples", &shadowSamples, 1, 32))
        {
            light.setShadowSamples(static_cast<uint32_t>(shadowSamples));
            changed = true;
            if (_onChangedFunction) _onChangedFunction();
        }
    }

    if (light.type() == base::Light::TypeSpot)
    {
        BasicWidgets::separator("Spot");
        float spotAngle = light.spotAngle();
        if (Input::sliderFloat("Spot Angle", &spotAngle, 1.0f, 90.0f))
        {
            light.setSpotAngle(spotAngle);
            changed = true;
            if (_onChangedFunction) _onChangedFunction();
        }

        float spotCutoff = light.spotCutoff();
        if (Input::sliderFloat("Spot Cutoff", &spotCutoff, 1.0f, 90.0f))
        {
            light.setSpotCutoff(spotCutoff);
            changed = true;
            if (_onChangedFunction) _onChangedFunction();
        }
    }

    return changed;
}

void LightEditor::cleanup()
{
    _lightComponent.reset();
}

}
