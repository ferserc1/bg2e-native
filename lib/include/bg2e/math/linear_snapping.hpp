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

#include <cmath>

namespace bg2e::math {

enum class LinearSnapMode { Relative, Absolute };

struct LinearSnapSettings {
    bool enabled = false;
    LinearSnapMode mode = LinearSnapMode::Relative;
    double step = 0.05;

    bool valid() const
    {
        return (mode == LinearSnapMode::Relative || mode == LinearSnapMode::Absolute) &&
            std::isfinite(step) && step > 0.0;
    }
};

// Capture once at gesture start. Evaluate every sample from the original
// position and the unsnapped signed displacement, in scene units.
class LinearSnapGesture {
  public:
    explicit LinearSnapGesture(LinearSnapSettings settings) : _settings(settings)
    {
    }

    double displacement(double startWorldCoordinate, double delta, bool worldAxisAligned) const
    {
        if (!_settings.enabled || !_settings.valid())
        {
            return delta;
        }
        const bool absolute = _settings.mode == LinearSnapMode::Absolute;
        if (absolute && !worldAxisAligned)
        {
            return delta;
        }

        const double origin = absolute ? startWorldCoordinate : 0.0;
        const double snapped = std::round((origin + delta) / _settings.step) * _settings.step - origin;
        return std::isfinite(snapped) ? snapped : delta;
    }

  private:
    LinearSnapSettings _settings;
};

}
