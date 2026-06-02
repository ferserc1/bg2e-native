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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/scene/CameraComponent.hpp>

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace bg2e {
namespace ui {

class BG2E_API CameraSettings {
public:
    virtual ~CameraSettings();

    void setCameraComponent(scene::CameraComponent* camera)
    {
        setCameraComponent(std::dynamic_pointer_cast<scene::CameraComponent>(camera->shared_from_this()));
    }

    void setCameraComponent(const std::shared_ptr<scene::CameraComponent>& camera)
    {
        _cameraComponent = camera;
        auto cameraComponent = _cameraComponent.lock();
        if (cameraComponent) {
            auto optical = cameraComponent->camera().projection<math::OpticalProjection>();
            if (optical) {
                _focalLength = optical->focalLength();
                _frameSize = optical->frameSize();
                _selectedSensorSize = getSensorIndex(_frameSize);
            } else {
                _focalLength = 35.0f;
                _frameSize = 35.0f;
                _selectedSensorSize = 1;
            }
        } else {
            _focalLength = 35.0f;
            _frameSize = 35.0f;
            _selectedSensorSize = 1;
        }
    }

    [[nodiscard]] std::weak_ptr<scene::CameraComponent> getCameraComponent() const
    {
        return _cameraComponent;
    }

    bool draw();

    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChangedFunction = cb; }

    static const std::vector<std::string>& sensorNames()
    {
        if (_sensorNames.empty()) {
            _sensorNames = {
                "APS-C (22mm)",
                "Full Frame (35mm)",
                "Medium Format (50mm)",
                "Large Format (75mm)"
            };
        }
        return _sensorNames;
    }

    static float sensorSize(uint32_t index)
    {
        switch (index) {
            case 0: return 22.0f;
            case 1: return 35.0f;
            case 2: return 50.0f;
            case 3: return 75.0f;
            default: return 35.0f;
        }
    }

    static uint32_t getSensorIndex(float frameSize)
    {
        float minDist = std::numeric_limits<float>::max();
        uint32_t bestIndex = 1;
        for (uint32_t i = 0; i < 4; ++i) {
            float dist = std::abs(sensorSize(i) - frameSize);
            if (dist < minDist) {
                minDist = dist;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

protected:
    std::weak_ptr<scene::CameraComponent> _cameraComponent;

    float _focalLength = 35.0f;
    float _frameSize = 35.0f;
    uint32_t _selectedSensorSize = 1;

    std::function<void()> _onChangedFunction;

    static std::vector<std::string> _sensorNames;
};

}
}