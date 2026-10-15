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

#include <bg2e/draw/environment/CubemapRenderer.hpp>

namespace bg2e::scene {
class EnvironmentComponent;
}
namespace bg2e::draw::environment {
struct EnvironmentGenerationConfiguration {
    std::shared_ptr<CubemapRenderer> sphere;
    std::shared_ptr<CubemapRenderer> defaultSky;
    std::shared_ptr<CubemapRenderer> irradiance;
    std::shared_ptr<CubemapRenderer> specular;
    std::shared_ptr<CubemapRenderer> brdf;
};
// Synchronous, public orchestration. Stop producers before build/synchronize.
// Failed generation leaves the last published resource snapshot intact.
class BG2E_API EnvironmentGenerator {
  public:
    EnvironmentGenerator(Engine& engine, const std::filesystem::path& shaderRoot);
    EnvironmentGenerationConfiguration& configuration()
    {
        return _configuration;
    }
    std::shared_ptr<const EnvironmentResources> build(const std::filesystem::path& hdrSource = {});
    std::shared_ptr<const EnvironmentResources> build(std::shared_ptr<gpu::Image> linearEquirectangular);
    void requestRebuild(); // Marks generation intent; synchronize consumes it at a safe boundary.
    bool synchronize(const scene::EnvironmentComponent* component);
    std::shared_ptr<const EnvironmentResources> resources() const
    {
        return _resources;
    }
    void cleanup();
  private:
    std::shared_ptr<const EnvironmentResources> generate(std::shared_ptr<gpu::Image> source);
    Engine* _engine;
    std::filesystem::path _shaderRoot;
    std::filesystem::path _sourcePath;
    bool _pathSource = false;
    bool _rebuildRequested = false;
    EnvironmentGenerationConfiguration _configuration;
    std::shared_ptr<const EnvironmentResources> _resources;
};
}
