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

#include <bg2e/utils/SceneCameraUtils.hpp>

#include <bg2e/math/projections.hpp>
#include <bg2e/render/ViewpointAnalyzer.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>

#include <cmath>
#include <limits>

namespace bg2e::utils {

float cameraVerticalFieldOfView(scene::CameraComponent * camera, float fallback)
{
    const auto validFieldOfView = [](float value)
    {
        return std::isfinite(value) && value > 0.0f && value < 179.0f;
    };

    if (!validFieldOfView(fallback))
    {
        fallback = 35.0f;
    }
    if (!camera || !camera->projection())
    {
        return fallback;
    }

    if (auto perspective = dynamic_cast<math::PerspectiveProjection *>(camera->projection()))
    {
        return validFieldOfView(perspective->fov()) ? perspective->fov() : fallback;
    }

    if (auto optical = dynamic_cast<math::OpticalProjection *>(camera->projection()))
    {
        if (optical->frameSize() > 0.0f &&
            optical->focalLength() > std::numeric_limits<float>::epsilon())
        {
            const float fieldOfView = glm::degrees(2.0f * std::atan(
                optical->frameSize() / (2.0f * optical->focalLength())
            ));
            return validFieldOfView(fieldOfView) ? fieldOfView : fallback;
        }
    }

    return fallback;
}

void applyViewpointToOrbitCamera(
    const render::CameraCandidate& candidate,
    scene::OrbitCameraComponent * orbitCamera,
    scene::CameraComponent * camera
)
{
    if (!orbitCamera)
    {
        return;
    }

    float orbitDistance = candidate.distance;
    auto orbitNode = orbitCamera->ownerNode();
    auto cameraNode = camera ? camera->ownerNode() : nullptr;
    if (orbitNode && cameraNode && orbitNode != cameraNode)
    {
        const glm::mat4 cameraFromOrbit = glm::inverse(orbitNode->worldMatrix()) * cameraNode->worldMatrix();
        orbitDistance -= cameraFromOrbit[3].z;
    }

    orbitCamera->setCenter(candidate.target);
    orbitCamera->setDistance(orbitDistance);
    orbitCamera->setRotation({ candidate.pitch, candidate.yaw });
}

}
