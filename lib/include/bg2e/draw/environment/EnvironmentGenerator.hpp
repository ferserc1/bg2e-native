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
