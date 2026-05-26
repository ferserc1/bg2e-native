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

#include <bg2e/ui/PolarTransformControllerEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>

namespace bg2e::ui {

PolarTransformControllerEditor::~PolarTransformControllerEditor() = default;

bool PolarTransformControllerEditor::draw()
{
    auto comp = _component.lock();
    if (!comp)
    {
        return false;
    }

    bool changed = false;

    BasicWidgets::separator("Position");

    float azimuth = comp->azimuth();
    if (Input::sliderFloat("Azimuth", &azimuth, 0.0f, 360.0f))
    {
        comp->setAzimuth(azimuth);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    float elevation = comp->elevation();
    if (Input::sliderFloat("Elevation", &elevation, -90.0f, 90.0f))
    {
        comp->setElevation(elevation);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    float distance = comp->distance();
    if (Input::sliderFloat("Distance", &distance, 0.0f, 50.0f))
    {
        comp->setDistance(distance);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    BasicWidgets::separator("Orientation");

    float eulerX = comp->eulerX();
    if (Input::sliderFloat("Euler X", &eulerX, -360.0f, 360.0f))
    {
        comp->setEulerX(eulerX);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    float eulerY = comp->eulerY();
    if (Input::sliderFloat("Euler Y", &eulerY, -360.0f, 360.0f))
    {
        comp->setEulerY(eulerY);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    float eulerZ = comp->eulerZ();
    if (Input::sliderFloat("Euler Z", &eulerZ, -360.0f, 360.0f))
    {
        comp->setEulerZ(eulerZ);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    BasicWidgets::separator("Target");

    glm::vec3 target = comp->target();
    if (Input::vec3("Target", target))
    {
        comp->setTarget(target);
        changed = true;
        if (_onChangedFunction) _onChangedFunction();
    }

    return changed;
}

void PolarTransformControllerEditor::cleanup()
{
    _component.reset();
}

}