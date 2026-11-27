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

#include <bg2e/math/linear_snapping.hpp>

#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>

namespace {

void expectNear(double actual, double expected)
{
    if (!(std::abs(actual - expected) < 1e-12))
    {
        throw std::runtime_error("Unexpected snapped displacement");
    }
}

}

int main()
{
    using namespace bg2e::math;
    constexpr double start = 0.123;
    LinearSnapSettings settings;
    expectNear(LinearSnapGesture(settings).displacement(start, 0.036, true), 0.036);
    settings.enabled = true;
    const LinearSnapGesture relative(settings);
    expectNear(start + relative.displacement(start, 0.05, true), 0.173);
    expectNear(relative.displacement(start, 0.025, false), 0.05);
    expectNear(relative.displacement(start, -0.025, false), -0.05);
    expectNear(relative.displacement(start, 0.076, true), 0.10);
    expectNear(relative.displacement(start, 0.012, true), 0.0);
    expectNear(relative.displacement(start, -0.076, true), -0.10);

    settings.mode = LinearSnapMode::Absolute;
    const LinearSnapGesture absolute(settings);
    expectNear(start + absolute.displacement(start, 0.036, true), 0.15);
    expectNear(absolute.displacement(start, 0.036, false), 0.036);
    expectNear(start + absolute.displacement(start, -0.200, true), -0.10);
    settings.step = 0.1;
    expectNear(relative.displacement(start, 0.04, true), 0.05);
    expectNear(absolute.displacement(start, 0.036, true), 0.027);

    for (const double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()})
    {
        settings.step = invalid;
        if (settings.valid())
        {
            throw std::runtime_error("An invalid step was accepted");
        }
        expectNear(LinearSnapGesture(settings).displacement(start, 0.036, true), 0.036);
    }
}
