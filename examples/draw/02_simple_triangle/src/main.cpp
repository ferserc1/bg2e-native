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

#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Mesh.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Window.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace bg2e;

// Shared application state is accessed exclusively on the main thread.
struct Controller {
    app::MainLoop& loop;
    base::Color color{0.12f, 0.18f, 0.28f, 1.0f};
    bool paused = false;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
};

struct PushConstants {
    float color[4];
};

class SceneDelegate final : public draw::RenderLoopDelegate {
  public:
    explicit SceneDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void initScene() override
    {
        auto* device = _engine->device();
        gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_simple_triangle",
                               _engine->backendType());
        _vertexShader = shaders.vertex("triangle", device);
        _fragmentShader = shaders.fragment("triangle", device);
        _computeShader = shaders.compute("gradient", device);

        gpu::PipelineLayoutDescription graphicsLayout{};
        graphicsLayout.pushConstants.push_back({0, sizeof(PushConstants), gpu::ShaderStage::Fragment});
        graphicsLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        graphicsLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        graphicsLayout.debugName = "Textured graphics layout";
        _graphicsLayout = device->createPipelineLayout(graphicsLayout);

        gpu::PipelineLayoutDescription computeLayout{};
        computeLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
        computeLayout.debugName = "Gradient compute layout";
        _computeLayout = device->createPipelineLayout(computeLayout);

        const std::array<std::array<uint8_t, 4>, 4> texels = {{
            {{255, 0, 0, 255}}, {{0, 255, 0, 255}},
            {{0, 0, 255, 255}}, {{255, 255, 0, 255}}
        }};
        _texture = device->createImage({.size = {2, 2},
                                       .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                       .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                       .debugName = "Procedural 2x2 texture"});
        _texture->uploadRGBA8(texels.data(), {2, 2});
        device->immediateSubmit([this](gpu::CommandBuffer* command)
        {
            command->transition(_texture.get(), gpu::ImageLayout::ShaderReadOnly);
        });
        _sampler = device->createSampler({.debugName = "Linear sampler"});
        _textureSet = device->createResourceSet(_graphicsLayout.get(), 0, "Pentagon texture");
        _textureSet->setSampledImage({.vulkan = 0, .metal = 0}, _texture.get());
        _textureSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
        _textureSet->update();
        _backgroundSet = device->createResourceSet(_graphicsLayout.get(), 0, "Gradient background");
        _computeSet = device->createResourceSet(_computeLayout.get(), 0, "Gradient output");

        // Build a regular pentagon mesh (position + texCoord0).
        // 5 perimeter vertices (radius 0.5, top vertex first) + 1 centre vertex.
        // Decomposed into a triangle fan: 5 triangles, 15 indices.
        geo::MeshPU meshData;
        meshData.vertices = {
            // perimeter (angle = -90° + i*72°)
            {{0.0000f, -0.5000f, 0.0f}, {0.500f, 0.000f}},  // 0 – top
            {{0.4755f, -0.1545f, 0.0f}, {0.976f, 0.345f}},  // 1 – upper-right
            {{0.2939f, 0.4045f, 0.0f}, {0.794f, 0.905f}},   // 2 – lower-right
            {{-0.2939f, 0.4045f, 0.0f}, {0.206f, 0.905f}},  // 3 – lower-left
            {{-0.4755f, -0.1545f, 0.0f}, {0.024f, 0.345f}}, // 4 – upper-left
            // centre
            {{0.0000f, 0.0000f, 0.0f}, {0.500f, 0.500f}}, // 5
        };
        meshData.indices = {
            5,
            0,
            1,
            5,
            1,
            2,
            5,
            2,
            3,
            5,
            3,
            4,
            5,
            4,
            0,
        };
        meshData.submeshes = {{0, 15}};

        _mesh.setMeshData(meshData);
        _mesh.build(device);

        // Draw the compute result with the same textured graphics pipeline.
        geo::MeshPU background;
        background.vertices = {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}}
        };
        background.indices = {0, 1, 2, 0, 2, 3};
        background.submeshes = {{0, 6}};
        _backgroundMesh.setMeshData(background);
        _backgroundMesh.build(device);

        gpu::ComputePipelineDescription computePipeline{};
        computePipeline.computeShader = _computeShader.get();
        computePipeline.layout = _computeLayout.get();
        computePipeline.debugName = "Gradient compute pipeline";
        _computePipeline = device->createComputePipeline(computePipeline);
    }

    void resize(gpu::Size2D extent) override
    {
        // draw drains in-flight frames before resize. Descriptor bindings remain
        // immutable between resizes, so one persistent set can serve every slot.
        if (_gradient)
        {
            _gradient->cleanup();
        }
        auto* device = _engine->device();
        _gradient = device->createImage({.size = extent,
                                        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                        .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled,
                                        .debugName = "Compute gradient"});
        _computeSet->setStorageImage({.vulkan = 0, .metal = 0}, _gradient.get());
        _computeSet->update();
        _backgroundSet->setSampledImage({.vulkan = 0, .metal = 0}, _gradient.get());
        _backgroundSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
        _backgroundSet->update();

        // Recreate for the current color format. draw's retained target has no depth.
        if (_pipeline)
        {
            _pipeline->cleanup();
        }
        gpu::GraphicsPipelineDescription pipeline{};
        pipeline.vertexShader = _vertexShader.get();
        pipeline.fragmentShader = _fragmentShader.get();
        pipeline.layout = _graphicsLayout.get();
        pipeline.colorFormat = _engine->surface()->colorFormat();
        pipeline.cullMode = gpu::CullMode::None;
        pipeline.debugName = "Textured background and pentagon";
        pipeline.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        _pipeline = device->createGraphicsPipeline(pipeline);
    }

    void update(const draw::FrameContext& context) override
    {
        // Scene time advances only while active; pause also freezes the tint.
        _time += context.deltaSeconds;
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        command.transition(_gradient.get(), gpu::ImageLayout::General);
        command.beginCompute();
        command.bindPipeline(_computePipeline.get());
        command.bindResourceSet(_computePipeline.get(), 0, _computeSet.get());
        command.dispatch((context.extent.width + 15) / 16,
                         (context.extent.height + 15) / 16, 1);
        command.endCompute();
        command.transition(_gradient.get(), gpu::ImageLayout::ShaderReadOnly);

        command.beginRendering(&context.colorTarget);
        command.bindPipeline(_pipeline.get());
        command.bindResourceSet(_pipeline.get(), 0, _backgroundSet.get());
        const auto& color = _controller->color;
        const PushConstants background{{color.r, color.g, color.b, color.a}};
        command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(background), &background);
        _backgroundMesh.draw(&command);

        const PushConstants tint{{
            0.5f + 0.5f * std::sin(_time * 1.3f),
            0.5f + 0.5f * std::sin(_time * 1.7f + 1.0f),
            0.5f + 0.5f * std::sin(_time * 2.1f + 2.0f),
            1.0f
        }};
        command.bindResourceSet(_pipeline.get(), 0, _textureSet.get());
        command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(tint), &tint);
        _mesh.draw(&command);
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        // draw waits for GPU completion before calling delegate cleanup, also
        // when initialization failed after creating only some resources.
        _mesh.cleanup();
        _backgroundMesh.cleanup();
        release(_computePipeline);
        release(_pipeline);
        release(_computeSet);
        release(_backgroundSet);
        release(_textureSet);
        release(_sampler);
        release(_gradient);
        release(_texture);
        release(_computeLayout);
        release(_graphicsLayout);
        release(_computeShader);
        release(_fragmentShader);
        release(_vertexShader);
    }

  private:
    template <typename T>
    static void release(std::shared_ptr<T>& resource)
    {
        if (resource)
        {
            resource->cleanup();
            resource.reset();
        }
    }

    std::shared_ptr<Controller> _controller;
    float _time = 0.0f;
    gpu::MeshPU _mesh;
    gpu::MeshPU _backgroundMesh;
    std::shared_ptr<gpu::ShaderModule> _vertexShader;
    std::shared_ptr<gpu::ShaderModule> _fragmentShader;
    std::shared_ptr<gpu::ShaderModule> _computeShader;
    std::shared_ptr<gpu::PipelineLayout> _graphicsLayout;
    std::shared_ptr<gpu::PipelineLayout> _computeLayout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ComputePipeline> _computePipeline;
    std::shared_ptr<gpu::Image> _texture;
    std::shared_ptr<gpu::Image> _gradient;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::shared_ptr<gpu::ResourceSet> _textureSet;
    std::shared_ptr<gpu::ResourceSet> _backgroundSet;
    std::shared_ptr<gpu::ResourceSet> _computeSet;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Simple triangle controls");
        _window.setPosition(20, 20);
        _window.setSize(380, 300);
        _window.options.noClose = true;
    }

    void drawUI() override
    {
        ++_controller->uiFrames;
        // draw retains the scene until invalidated. Request animation frames
        // from the always-active UI path without advancing the paused scene.
        if (!_controller->paused)
        {
            _controller->loop.requestSceneFrame();
        }
        _window.draw([this]
         {
             auto& state = *_controller;
             ui::Text::text("Backend: " + state.backend);
             ui::Text::text("UI frames: " + std::to_string(state.uiFrames));
             ui::Text::text("Scene updates: " + std::to_string(state.sceneFrames));
             if (ui::Value::colorPicker("Background", state.color))
             {
                 state.loop.requestSceneFrame();
             }
             if (ui::Button::checkBox("Pause scene", &state.paused))
             {
                 if (state.paused)
                 {
                     state.loop.pauseScene({state.color.r, state.color.g, state.color.b, state.color.a});
                 }
                 else
                 {
                     state.loop.resumeScene();
                 }
             }
             if (ui::Button::button("Refresh scene"))
             {
                 state.loop.requestSceneFrame();
             }
             if (state.paused)
             {
                 ui::Text::text("Scene changes remain pending until resumed.");
             }
         });
    }

  private:
    std::shared_ptr<Controller> _controller;
    ui::Window _window;
};

class Application final : public app::Application {
  public:
    explicit Application(app::MainLoop& loop)
        : _controller(std::make_shared<Controller>(Controller{loop}))
    {
    }

    void init(int argc, char** argv) override
    {
        bool backendSpecified = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument != "--backend=vulkan" && argument != "--backend=metal")
            {
                throw std::invalid_argument("Unknown argument '" + argument +
                                            "'. Use --backend=vulkan or --backend=metal.");
            }
            if (backendSpecified)
            {
                throw std::invalid_argument("Specify --backend only once.");
            }
            backendSpecified = true;
            _config.backend = argument == "--backend=metal"
                                  ? gpu::BackendType::Metal
                                  : gpu::BackendType::Vulkan;
        }
#ifndef BG2E_IS_MAC
        if (_config.backend == gpu::BackendType::Metal)
        {
            throw std::invalid_argument("Metal is available only on macOS. Use --backend=vulkan.");
        }
#endif
        _controller->backend = _config.backend == gpu::BackendType::Metal ? "Metal" : "Vulkan";
        setRenderDelegate(std::make_shared<SceneDelegate>(_controller));
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>(_controller));
    }

    const draw::EngineConfig& config() const
    {
        return _config;
    }

  private:
    draw::EngineConfig _config;
    std::shared_ptr<Controller> _controller;
};
}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.simple-triangle");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: simple triangle", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_simple_triangle: " << error.what() << '\n';
        return 1;
    }
}
