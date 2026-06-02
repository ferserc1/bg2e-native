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

#include "bg2e/ui/CameraSettings.hpp"

#include "bg2e/ui/Input.hpp"
#include "bg2e/ui/BasicWidgets.hpp"

namespace bg2e {
namespace ui {

std::vector<std::string> CameraSettings::_sensorNames;

CameraSettings::~CameraSettings()
{
}

bool CameraSettings::draw()
{
    bool changed = false;

    auto opt = _cameraComponent.lock();
    if (!opt) {
        return false;
    }

    auto optical = opt->camera().projection<math::OpticalProjection>();

    if (!optical) {
        if (BasicWidgets::button("Configure Projection")) {
            auto proj = new math::OpticalProjection();
            proj->setFocalLength(_focalLength);
            proj->setFrameSize(_frameSize);
            proj->setFar(1000.0f);
            opt->camera().setProjection(proj);
            changed = true;
            if (_onChangedFunction) {
                _onChangedFunction();
            }
        }
        return changed;
    }

    if (Input::sliderFloat("Focal Length", &_focalLength, 18.0f, 200.0f)) {
        optical->setFocalLength(_focalLength);
        changed = true;
    }

    auto names = sensorNames();
    if (Input::comboBox("Sensor Size", names, _selectedSensorSize)) {
        _frameSize = sensorSize(_selectedSensorSize);
        optical->setFrameSize(_frameSize);
        changed = true;
    }

    if (changed && _onChangedFunction) {
        _onChangedFunction();
    }

    return changed;
}

void CameraSettings::cleanup()
{
    _cameraComponent.reset();
}

}
}