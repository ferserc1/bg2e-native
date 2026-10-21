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

#include "uv_surface/probe.hpp"
#include "integrated/probe.hpp"
#include "output/probe.hpp"
#include "retirement_uv.hpp"
#include "retirement_numeric.hpp"
#include "retirement_material.hpp"
#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <string_view>

namespace bg2e::bake::tests {

class RetirementProbe final : public app::DrawOffscreenApplicationDelegate {
  public:
    explicit RetirementProbe(bool integrated) : _integrated(integrated)
    {
    }

    void initConfig(int argc, char** argv, app::DrawOffscreenApplicationConfig& config) override
    {
        if (argc > 3)
        {
            throw std::invalid_argument("Expected [vulkan|metal] [shader-root]");
        }
        if (argc > 1)
        {
            const std::string_view backend(argv[1]);
            if (backend != "vulkan" && backend != "metal")
            {
                throw std::invalid_argument("Expected vulkan or metal backend");
            }
            config.engine.backend = backend == "metal" ? gpu::BackendType::Metal : gpu::BackendType::Vulkan;
        }
        _shaders = argc > 2 ? std::filesystem::path(argv[2]) : base::PlatformTools::shaderPath();
        config.extent = {1, 1};
        config.createDepthImage = false;
    }

    void init(draw::Engine* engine, gpu::OffscreenSurface*) override
    {
        _engine = engine;
    }
    bool continueRendering() const override
    {
        return false;
    }
    void render(const draw::FrameContext&) override
    {
        throw std::logic_error("Bake probes do not render camera frames");
    }

    void finish() override
    {
        runUvSurfaceGpuProbe(*_engine, _shaders);
        runRetirementUvProbe(*_engine, _shaders);
        if (!_integrated)
        {
            return;
        }
        runRetirementMaterialProbe(*_engine, _shaders);
        runBakePixelConversionProbe();
        runBakeOutputGpuProbe(*_engine->device(), _engine->backendType(), _shaders);
        draw::environment::EnvironmentGenerator generator(*_engine, _shaders);
        gpu::ImageDescription skyDescription;
        skyDescription.size = {8, 4};
        skyDescription.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
        skyDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        auto sky = _engine->device()->createImage(skyDescription);
        std::vector<float> skyPixels(8 * 4 * 4);
        for (size_t i = 0; i < skyPixels.size(); i += 4)
        {
            skyPixels[i] = .45f;
            skyPixels[i + 1] = .65f;
            skyPixels[i + 2] = .8f;
            skyPixels[i + 3] = 1;
        }
        sky->upload({skyPixels.data(), skyPixels.size() * sizeof(float)});
        auto environment = generator.build(sky);
        runRetirementNumericProbe(*_engine, _shaders, environment);
        runBakeGiCompositionProbe(*_engine->device(), _engine->backendType(), _shaders, environment);
        auto resource = std::make_shared<draw::Drawable>();
        resource->setMesh(std::make_shared<geo::Mesh>(uvSurfaceQuad()));
        resource->load(_engine);
        auto target = std::make_shared<scene::Node>("UV bake target");
        target->addComponent(std::make_shared<scene::DrawableComponent>(resource));
        auto root = std::make_shared<scene::Node>("Bake probe scene");
        root->addChild(target);
        runIntegratedBakeProbe(*_engine, root, target, environment, _shaders, LightmapMode::RTAO);
        runIntegratedBakeProbe(*_engine, root, target, environment, _shaders, LightmapMode::RTGI);
        // Local contexts and native resource owners release while Engine is alive.
    }

  private:
    bool _integrated;
    draw::Engine* _engine = nullptr;
    std::filesystem::path _shaders;
};

inline int runRetirementProbe(int argc, char** argv, bool integrated)
{
    runUvSurfaceCpuProbe();
    app::OffscreenApplication application;
    application.init(argc, argv, "org.bg2e.tests.bake", std::make_shared<RetirementProbe>(integrated));
    return application.run();
}

}
