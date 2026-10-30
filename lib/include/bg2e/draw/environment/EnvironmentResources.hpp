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

#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <memory>
#include <bg2e/math/base.hpp>
#include <bg2e/base/SolarLight.hpp>

namespace bg2e {
namespace gpu {
class Sampler;
class Device;
}
namespace draw {
namespace environment {

// Linear HDR radiance, irradiance (includes PI), GGX prefilter and split-sum RG LUT.
// Images/samplers retain shared ownership; Device/Engine must outlive all users.
struct BG2E_API EnvironmentResources {
    std::shared_ptr<gpu::Image> sky;
    // Optional disk-free background for a screen-resolution analytic solar disk.
    std::shared_ptr<gpu::Image> skyboxBackground;
    base::SolarState solar;
    std::shared_ptr<gpu::Image> irradiance;
    std::shared_ptr<gpu::Image> prefilteredSpecular;
    std::shared_ptr<gpu::Image> brdf;
    std::shared_ptr<gpu::Sampler> sampler;
    std::shared_ptr<gpu::Sampler> brdfSampler;
    float skyboxBlur = 0.0f; // Normalized prefiltered roughness, [0, 1].
    bool skyboxVisible = true;
    void validate(gpu::Device* device) const;
};

// The same public recording contract supports standalone environment work.
// Sphere/cube conversion, irradiance, prefilter and BRDF implementations derive
// from this family. Shader selection belongs to each concrete tool.
class BG2E_API EnvironmentPass : public pipeline::RenderLayer {
  public:
    virtual const EnvironmentResources& resources() const = 0;
};

class BG2E_API SkyboxPass : public pipeline::RenderLayer {
  public:
    virtual void setCamera(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void setEnvironment(std::shared_ptr<const EnvironmentResources> resources) = 0;
};

}
}
}
