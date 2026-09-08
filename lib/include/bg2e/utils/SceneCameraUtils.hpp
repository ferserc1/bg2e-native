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

namespace bg2e::scene {
class CameraComponent;
class OrbitCameraComponent;
}

namespace bg2e::render {
struct CameraCandidate;
}

namespace bg2e::utils {

/** Returns the camera's vertical FOV in degrees, or fallback if unavailable. */
BG2E_API float cameraVerticalFieldOfView(
    scene::CameraComponent * camera,
    float fallback = 35.0f
);

/**
 * Applies an analyzed camera candidate to an orbit controller. The distance is
 * adjusted when the CameraComponent is on a translated child of the orbit node.
 */
BG2E_API void applyViewpointToOrbitCamera(
    const render::CameraCandidate& candidate,
    scene::OrbitCameraComponent * orbitCamera,
    scene::CameraComponent * camera
);

}
