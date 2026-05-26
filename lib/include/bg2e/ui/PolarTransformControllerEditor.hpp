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
#include <bg2e/scene/PolarTransformController.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace scene {
    class PolarTransformControllerComponent;
}
namespace ui {

class BG2E_API PolarTransformControllerEditor {
public:
    virtual ~PolarTransformControllerEditor();

    void setComponent(scene::PolarTransformControllerComponent * comp)
    {
        setComponent(std::dynamic_pointer_cast<scene::PolarTransformControllerComponent>(comp->shared_from_this()));
    }

    void setComponent(const std::shared_ptr<scene::PolarTransformControllerComponent> & component)
    {
        _component = component;
    }

    [[nodiscard]] std::weak_ptr<scene::PolarTransformControllerComponent> getComponent() const
    {
        return _component;
    }

    bool draw();

    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChangedFunction = cb; }

protected:
    std::weak_ptr<scene::PolarTransformControllerComponent> _component;

    std::function<void()> _onChangedFunction;
};

}
}