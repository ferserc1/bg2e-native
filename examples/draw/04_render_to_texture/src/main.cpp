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
#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/gpu/FrameResourceRing.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/math/base.hpp>
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

struct CameraUBO {
    glm::mat4 projectionView{1.0f};
};

struct ModelUBO {
    glm::mat4 model{1.0f};
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
        // Keep static DeviceResource teardown local to this delegate.
        _cleanup = std::make_unique<gpu::CleanupManager>(_engine->surface());
        gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_render_to_texture",
                               _engine->backendType());
        _vertexShader = shaders.vertex("cube", device);
        _cleanup->push(_vertexShader);
        _fragmentShader = shaders.fragment("cube", device);
        _cleanup->push(_fragmentShader);
        _computeShader = shaders.compute("edge_filter", device);
        _cleanup->push(_computeShader);
        _outputVertexShader = shaders.vertex("output", device);
        _cleanup->push(_outputVertexShader);
        _outputFragmentShader = shaders.fragment("output", device);
        _cleanup->push(_outputFragmentShader);

        gpu::PipelineLayoutDescription layout{};
        layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        layout.resourceBindings.push_back({1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        layout.resourceBindings.push_back({2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({2, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        layout.debugName = "Cube pipeline layout";
        _layout = device->createPipelineLayout(layout);
        _cleanup->push(_layout);

        gpu::PipelineLayoutDescription computeLayout{};
        computeLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
        computeLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
        computeLayout.debugName = "Sobel compute layout";
        _computeLayout = device->createPipelineLayout(computeLayout);
        _cleanup->push(_computeLayout);
        gpu::ComputePipelineDescription computePipeline{};
        computePipeline.computeShader = _computeShader.get();
        computePipeline.layout = _computeLayout.get();
        computePipeline.debugName = "Sobel compute pipeline";
        _computePipeline = device->createComputePipeline(computePipeline);
        _cleanup->push(_computePipeline);

        gpu::PipelineLayoutDescription outputLayout{};
        outputLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        outputLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        outputLayout.debugName = "Fullscreen output layout";
        _outputLayout = device->createPipelineLayout(outputLayout);
        _cleanup->push(_outputLayout);

        const std::array<std::array<uint8_t, 4>, 4> texels = {{
            {{255, 0, 0, 255}}, {{0, 255, 0, 255}},
            {{0, 0, 255, 255}}, {{255, 255, 0, 255}}
        }};
        _texture = device->createImage({.size = {2, 2},
                                       .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                       .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                       .debugName = "Procedural 2x2 texture"});
        _cleanup->push(_texture);
        _texture->uploadRGBA8(texels.data(), {2, 2});
        device->immediateSubmit([this](gpu::CommandBuffer* command)
        {
            command->transition(_texture.get(), gpu::ImageLayout::ShaderReadOnly);
        });
        _sampler = device->createSampler({.debugName = "Linear sampler"});
        _cleanup->push(_sampler);
        _textureSet = device->createResourceSet(_layout.get(), 2, "Material resource set");
        _cleanup->push(_textureSet);
        _textureSet->setSampledImage({.vulkan = 0, .metal = 0}, _texture.get());
        _textureSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
        _textureSet->update();

        // Camera contents change only during resize, after draw drains GPU work.
        _cameraUbo = device->createBuffer("Camera UBO");
        _cleanup->push(_cameraUbo);
        _cameraUbo->createUniformBuffer(CameraUBO{});
        _cameraSet = device->createResourceSet(_layout.get(), 0, "Camera resource set");
        _cleanup->push(_cameraSet);
        _cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, _cameraUbo);
        _cameraSet->update();

        std::unique_ptr<geo::MeshPU> cubeData(geo::createCubePU(1.0f, 1.0f, 1.0f));
        _cube.setMeshData(*cubeData);
        _cube.build(device);
    }

    void resize(gpu::Size2D extent) override
    {
        // This also runs before the first scene render. draw waits for all
        // in-flight work before replacing the retained target and calling us.
        auto* device = _engine->device();
        auto* surface = _engine->surface();
        cleanupTargetResources();
        cleanupModelResources();

        const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        const auto projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        const auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                      glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
        CameraUBO cameraData{};
        cameraData.projectionView = projection * view;
        _cameraUbo->updateUniformBuffer(cameraData);

        // The cube and Sobel pass use RGBA8, independent of the retained
        // target's format. Reuse the color image for the final sampled result.
        _offscreenColor = device->createImage({.size = extent,
                                              .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                              .usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Storage |
                                                       gpu::ImageUsage::TransferDst | gpu::ImageUsage::Sampled,
                                              .debugName = "Offscreen cube color and copied result"});
        _depth = device->createImage({.size = extent,
                                     .format = gpu::PixelFormat::D32_SFLOAT,
                                     .usage = gpu::ImageUsage::DepthStencil,
                                     .debugName = "Offscreen cube depth"});
        _computeOutput = device->createImage({.size = extent,
                                             .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                             .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc,
                                             .debugName = "Sobel output"});
        _computeSet = device->createResourceSet(_computeLayout.get(), 0, "Sobel images");
        _computeSet->setStorageImage({.vulkan = 0, .metal = 0}, _offscreenColor.get());
        _computeSet->setStorageImage({.vulkan = 1, .metal = 1}, _computeOutput.get());
        _computeSet->update();
        _outputSet = device->createResourceSet(_outputLayout.get(), 0, "Fullscreen output image");
        _outputSet->setSampledImage({.vulkan = 0, .metal = 0}, _offscreenColor.get());
        _outputSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
        _outputSet->update();

        _modelUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Model UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(ModelUBO{});
            return buffer;
        });
        _modelSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_layout.get(), 1,
                                                 "Model resource set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 3}, _modelUboRing.sharedAt(slot));
            set->update();
            return set;
        });

        gpu::GraphicsPipelineDescription pipeline{};
        pipeline.vertexShader = _vertexShader.get();
        pipeline.fragmentShader = _fragmentShader.get();
        pipeline.layout = _layout.get();
        pipeline.colorFormat = _offscreenColor->pixelFormat();
        pipeline.depthFormat = _depth->pixelFormat();
        pipeline.debugName = "Textured cube pipeline";
        pipeline.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        _pipeline = device->createGraphicsPipeline(pipeline);

        gpu::GraphicsPipelineDescription outputPipeline{};
        outputPipeline.vertexShader = _outputVertexShader.get();
        outputPipeline.fragmentShader = _outputFragmentShader.get();
        outputPipeline.layout = _outputLayout.get();
        outputPipeline.colorFormat = surface->colorFormat();
        outputPipeline.cullMode = gpu::CullMode::None;
        outputPipeline.debugName = "Fullscreen processed output";
        // No vertex buffers or depth: the shader generates a fullscreen triangle.
        _outputPipeline = device->createGraphicsPipeline(outputPipeline);
    }

    void update(const draw::FrameContext& context) override
    {
        // beginFrame has waited for this slot. Do not overwrite another slot's
        // model buffer while the GPU could still be reading it.
        _time += context.deltaSeconds;
        ModelUBO modelData{};
        modelData.model = glm::rotate(glm::mat4(1.0f), _time,
                                      glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));
        _modelUboRing.at(context.frameSlot)->updateUniformBuffer(modelData);
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        // Pass 1: render the rotating cube into a separate RGBA8 image.
        command.transition(_offscreenColor.get(), gpu::ImageLayout::ColorAttachment);
        command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
        command.beginRendering(_offscreenColor.get(), _depth.get());
        const auto& color = _controller->color;
        command.clearColor(0, {color.r, color.g, color.b, color.a});
        command.clearDepth(1.0f);
        command.bindPipeline(_pipeline.get());
        command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
        command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
        command.bindResourceSet(_pipeline.get(), 2, _textureSet.get());
        _cube.draw(&command);
        command.endRendering();

        // Pass 2: Sobel reads the rendered image and writes a separate output.
        command.transition(_offscreenColor.get(), gpu::ImageLayout::General);
        command.transition(_computeOutput.get(), gpu::ImageLayout::General);
        command.beginCompute();
        command.bindPipeline(_computePipeline.get());
        command.bindResourceSet(_computePipeline.get(), 0, _computeSet.get());
        command.dispatch((context.extent.width + 15) / 16,
                         (context.extent.height + 15) / 16, 1);
        command.endCompute();

        // Pass 3: demonstrate a same-format image copy. The unfiltered color
        // is no longer needed, so reuse its storage instead of a fourth image.
        command.transition(_computeOutput.get(), gpu::ImageLayout::TransferSrc);
        command.transition(_offscreenColor.get(), gpu::ImageLayout::TransferDst);
        command.copyImage(_computeOutput.get(), _offscreenColor.get());

        // Pass 4: sample into draw's retained color target. That target has
        // no TransferDst usage and may have a different format (e.g. BGRA8).
        command.transition(_offscreenColor.get(), gpu::ImageLayout::ShaderReadOnly);
        command.beginRendering(&context.colorTarget);
        command.bindPipeline(_outputPipeline.get());
        command.bindResourceSet(_outputPipeline.get(), 0, _outputSet.get());
        command.draw(3);
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        // draw drains GPU work before this callback, including partial init.
        cleanupTargetResources();
        cleanupModelResources();
        _cube.cleanup();
        if (_cleanup)
        {
            _cleanup->flush();
            _cleanup.reset();
        }
        _cameraSet.reset();
        _cameraUbo.reset();
        _textureSet.reset();
        _sampler.reset();
        _texture.reset();
        _outputLayout.reset();
        _computePipeline.reset();
        _computeLayout.reset();
        _layout.reset();
        _outputFragmentShader.reset();
        _outputVertexShader.reset();
        _computeShader.reset();
        _fragmentShader.reset();
        _vertexShader.reset();
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

    void cleanupTargetResources()
    {
        release(_outputPipeline);
        release(_pipeline);
        release(_outputSet);
        release(_computeSet);
        release(_computeOutput);
        release(_depth);
        release(_offscreenColor);
    }

    void cleanupModelResources()
    {
        _modelSetRing.cleanup();
        _modelSetRing.clear();
        _modelUboRing.cleanup();
        _modelUboRing.clear();
    }

    std::shared_ptr<Controller> _controller;
    float _time = 0.0f;
    gpu::MeshPU _cube;
    gpu::FrameResourceRing<gpu::Buffer> _modelUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _modelSetRing;
    std::unique_ptr<gpu::CleanupManager> _cleanup;
    std::shared_ptr<gpu::ShaderModule> _computeShader;
    std::shared_ptr<gpu::ShaderModule> _outputVertexShader;
    std::shared_ptr<gpu::ShaderModule> _outputFragmentShader;
    std::shared_ptr<gpu::PipelineLayout> _computeLayout;
    std::shared_ptr<gpu::PipelineLayout> _outputLayout;
    std::shared_ptr<gpu::ComputePipeline> _computePipeline;
    std::shared_ptr<gpu::GraphicsPipeline> _outputPipeline;
    std::shared_ptr<gpu::Image> _offscreenColor;
    std::shared_ptr<gpu::Image> _computeOutput;
    std::shared_ptr<gpu::ResourceSet> _computeSet;
    std::shared_ptr<gpu::ResourceSet> _outputSet;
    std::shared_ptr<gpu::ShaderModule> _vertexShader;
    std::shared_ptr<gpu::ShaderModule> _fragmentShader;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::Image> _texture;
    std::shared_ptr<gpu::Image> _depth;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::shared_ptr<gpu::ResourceSet> _textureSet;
    std::shared_ptr<gpu::Buffer> _cameraUbo;
    std::shared_ptr<gpu::ResourceSet> _cameraSet;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Render to texture controls");
        _window.setPosition(20, 20);
        _window.setSize(380, 330);
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
             ui::Text::text("Offscreen cube -> Sobel -> copy -> output");
             ui::Text::text("Background affects input edge contrast.");
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.render-to-texture");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: render to texture", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_render_to_texture: " << error.what() << '\n';
        return 1;
    }
}
