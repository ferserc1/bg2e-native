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

#include <bg2e/scene/Component.hpp>
#include <bg2e/base/Color.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace tutorial {

struct SampleValues {
    bool active = true;
    int32_t count = 2;
    uint32_t mask = 7;
    float weight = 0.5f;
    double precision = 0.125;
    std::string title = "Reflection playground";
    glm::vec2 uv = glm::vec2(1.0f);
    glm::vec3 offset = glm::vec3(0.0f);
    glm::vec4 vector = glm::vec4(1.0f);
    glm::mat4 matrix = glm::mat4(1.0f);
    bg2e::base::Color tint = bg2e::base::Color::White();
    std::filesystem::path path;
    std::filesystem::path image;
    std::string imageName;
};

struct SampleMode {
    virtual ~SampleMode() = default;
    bool active = true;
};

struct SmoothMode final : SampleMode {
    float smoothing = 0.5f;
};

struct StepMode final : SampleMode {
    uint32_t steps = 4;
};

class ReflectionSamplesComponent final : public bg2e::scene::Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("tutorial.ReflectionSamples");

    ReflectionSamplesComponent()
    {
        setIgnoreSerialization(true);
    }

    const SampleValues& values() const
    {
        return _values;
    }
    SampleValues& editableValues()
    {
        return _values;
    }
    const SampleMode* mode() const
    {
        return _mode.get();
    }
    SampleMode* editableMode()
    {
        return _mode.get();
    }
    void setMode(std::shared_ptr<SampleMode> mode)
    {
        _mode = std::move(mode);
    }
    std::string description() const
    {
        return "Sample values only; these do not affect rendering";
    }

    void reset()
    {
        _values = SampleValues{};
    }

    std::shared_ptr<bg2e::scene::Component> clone() const override
    {
        auto copy = std::make_shared<ReflectionSamplesComponent>();
        copy->_values = _values;
        copy->setPriority(priority());
        copy->setIgnoreSerialization(ignoreSerialization());
        if (const auto* smooth = dynamic_cast<const SmoothMode*>(_mode.get()))
        {
            copy->_mode = std::make_shared<SmoothMode>(*smooth);
        }
        else if (const auto* step = dynamic_cast<const StepMode*>(_mode.get()))
        {
            copy->_mode = std::make_shared<StepMode>(*step);
        }
        else
        {
            copy->_mode.reset();
        }
        return copy;
    }

  private:
    SampleValues _values;
    std::shared_ptr<SampleMode> _mode = std::make_shared<SmoothMode>();
};

}
