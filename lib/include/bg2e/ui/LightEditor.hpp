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
#include <bg2e/base/Light.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace scene {
    class LightComponent;
}
namespace ui {

class BG2E_API LightEditor {
public:
    virtual ~LightEditor();

    void setIntensityRange(float min, float max);

    void setLightComponent(std::shared_ptr<scene::LightComponent> light)
    {
        _lightComponent = light;
    }

    [[nodiscard]] std::weak_ptr<scene::LightComponent> getLightComponent() const
    {
        return _lightComponent;
    }

    bool draw();

    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChangedFunction = cb; }

protected:
    std::weak_ptr<scene::LightComponent> _lightComponent;

    float _intensityMin = 0.0f;
    float _intensityMax = 30.0f;

    std::function<void()> _onChangedFunction;
};

}
}
