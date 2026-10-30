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

#include <bg2e/base/SolarLight.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace bg2e::base {
SolarState evaluateSolarLight(const Light& light, const glm::vec3& rayDirection, bool atmosphere)
{
    SolarState state;
    if (light.type() != Light::TypeSun)
    {
        return state;
    }
    const float length = glm::length(rayDirection);
    const double distanceKm = double(light.solarDistanceMillionKm()) * 1.0e6;
    const double radiusKm = light.solarRadiusKm();
    const auto& color = light.color();
    if (!std::isfinite(length) || length < 1.0e-12f || !std::isfinite(distanceKm) ||
        !std::isfinite(radiusKm) || radiusKm <= 0.0 || distanceKm <= radiusKm ||
        !std::isfinite(light.intensity()) || light.intensity() < 0.0f ||
        !std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b) ||
        color.r < 0.0f || color.g < 0.0f || color.b < 0.0f)
    {
        throw std::invalid_argument("Solar light requires a valid direction, nonnegative emission and distance greater than radius");
    }
    state.enabled = true;
    state.direction = -rayDirection / length;
    state.angularRadius = float(std::asin(radiusKm / distanceKm));
    const double distanceScale = std::pow(149.6 / light.solarDistanceMillionKm(), 2.0);
    glm::vec3 transmission{1.0f};
    float visibility = 1.0f;
    if (atmosphere)
    {
        // Bruneton terrestrial approximation: 6360 km planet, 60 km shell,
        // exponential Rayleigh (8 km) and aerosol (1.2 km) density profiles.
        // RGB extinction approximates wavelengths 680, 550 and 440 nm.
        constexpr double planet = 6360.0;
        constexpr double top = planet + 60.0;
        constexpr double observer = planet + 0.001;
        const double mu = std::max(double(state.direction.y), 0.0);
        const double path = -observer * mu + std::sqrt(observer * observer * (mu * mu - 1.0) + top * top);
        constexpr int samples = 64;
        const double step = path / samples;
        double rayleigh = 0.0;
        double mie = 0.0;
        for (int i = 0; i < samples; ++i)
        {
            const double t = (i + 0.5) * step;
            const double height = std::max(std::sqrt(observer * observer + t * t + 2.0 * observer * mu * t) - planet, 0.0);
            rayleigh += std::exp(-height / 8.0) * step;
            mie += std::exp(-height / 1.2) * step;
        }
        const glm::vec3 opticalDepth = glm::vec3(0.0058f, 0.0136f, 0.0331f) * float(rayleigh) + glm::vec3(0.00444f * float(mie));
        transmission = glm::exp(-opticalDepth);
        const float elevation = std::asin(std::clamp(state.direction.y, -1.0f, 1.0f));
        const float fraction = std::clamp(elevation / state.angularRadius, -1.0f, 1.0f);
        visibility = (std::acos(-fraction) + fraction * std::sqrt(std::max(1.0f - fraction * fraction, 0.0f))) / std::numbers::pi_v<float>;
    }
    state.color = glm::vec3(color.r, color.g, color.b) * transmission;
    state.skyIntensity = float(double(light.intensity()) * distanceScale);
    state.intensity = state.skyIntensity * visibility;
    // Uniform disk: irradiance on a plane normal to the sun is L * pi*sin(a)^2.
    const double projectedSolidAngle = std::numbers::pi * std::pow(radiusKm / distanceKm, 2.0);
    state.diskRadiance = state.color * float(double(state.skyIntensity) / projectedSolidAngle);
    for (int i = 0; i < 3; ++i)
    {
        if (!std::isfinite(state.diskRadiance[i]))
        {
            throw std::invalid_argument("Solar radiance exceeds the supported floating point range");
        }
    }
    if (!std::isfinite(state.skyIntensity))
    {
        throw std::invalid_argument("Solar intensity exceeds the supported floating point range");
    }
    return state;
}
}
