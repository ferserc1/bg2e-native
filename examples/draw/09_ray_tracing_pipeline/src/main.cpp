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
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/gpu/FrameResourceRing.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/gpu/RayTracingScene.hpp>
#include <bg2e/gpu/RayTracingPipeline.hpp>
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

#include <vector>
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
    base::Color color{0.0f, 0.0f, 0.0f, 1.0f};
    bool paused = false;
    bool resetRequested = false;
    uint32_t samples = 0;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
    std::string deviceName;
};

struct CameraData {
    glm::mat4 viewInverse{1.0f};
    glm::mat4 projInverse{1.0f};
    glm::vec4 misc{0.0f}; // x=sampleCount, y=width, z=height
    glm::vec4 background{0.0f};
};

// Per-instance surface description consumed by the path tracer. Normals are
// computed analytically in the shader (the Cornell scene is made of planes, a
// sphere and an axis-aligned cube), so no per-vertex data needs to be bound.
//   shapeType: 0 = plane, 1 = sphere, 2 = cube
struct InstanceData {
    glm::vec4 albedo;         // rgb albedo, a = roughness
    glm::vec4 emission;       // rgb emission, a = shapeType
    glm::vec4 normalOrCenter; // plane: world normal; sphere/cube: world center; w = sphere radius
    glm::vec4 extents;        // cube: world-space half-extents
};

enum class ShapeType : int
{
    Plane = 0,
    Sphere = 1,
    Cube = 2
};

static_assert(sizeof(CameraData) == 160);
static_assert(sizeof(InstanceData) == 64);

struct ScenePiece {
    std::shared_ptr<gpu::MeshPN> mesh;
    glm::mat4 model{1.0f};
    InstanceData instance{};
    std::vector<std::shared_ptr<gpu::RayTracingMesh>> rtMeshes;
};

class SceneDelegate final : public draw::RenderLoopDelegate {
  public:
    explicit SceneDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void initScene() override
    {
        const auto properties = _engine->physicalDevice()->properties();
        if (!properties->rayTracingSupported())
        {
            std::string message = "This example requires a ray tracing capable GPU. Device: " + properties->name;
#ifdef BG2E_IS_MAC
            if (_engine->backendType() == gpu::BackendType::Vulkan)
            {
                message += ". Try --backend=metal on a compatible Mac.";
            }
#endif
            throw std::runtime_error(message);
        }
        _controller->deviceName = properties->name;
        auto* device = _engine->device();
        _cleanup = std::make_unique<gpu::CleanupManager>(_engine->surface());
        gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_ray_tracing_pipeline",
                               _engine->backendType());
        auto rgen = shaders.rayGeneration("ray_tracing_pipeline", device);
        auto rmiss = shaders.miss("ray_tracing_pipeline", device);
        auto rchit = shaders.closestHit("ray_tracing_pipeline", device);
        _cleanup->push(rgen);
        _cleanup->push(rmiss);
        _cleanup->push(rchit);

        gpu::PipelineLayoutDescription layout{};
        layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::RayGeneration, 1});
        layout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::RayGeneration, 1});
        layout.resourceBindings.push_back({0, {.vulkan = 2, .metal = 1}, gpu::ResourceType::StorageImage, gpu::ShaderStage::RayGeneration, 1});
        layout.resourceBindings.push_back({1, {.vulkan = 0, .metal = 2}, gpu::ResourceType::AccelerationStructure, gpu::ShaderStage::RayGeneration, 1});
        layout.resourceBindings.push_back({2, {.vulkan = 0, .metal = 3}, gpu::ResourceType::StorageBuffer, gpu::ShaderStage::ClosestHit, 1});
        layout.debugName = "Cornell box RT layout";
        _rtLayout = device->createPipelineLayout(layout);
        _cleanup->push(_rtLayout);

        std::vector<InstanceData> allInstanceData;

        // shapeParam: sphere -> (unused, unused, unused, radius); cube -> (halfX, halfY, halfZ, 0)
        auto addPiece = [&](bg2e::geo::MeshPN* data, const glm::mat4& model,
                            ShapeType shapeType, float roughness,
                            const glm::vec3& albedo, const glm::vec3& emission,
                            const glm::vec4& shapeParam = glm::vec4(0.0f))
        {
            std::unique_ptr<geo::MeshPN> meshData(data);
            auto piece = std::make_shared<ScenePiece>();
            _pieces.push_back(piece);
            piece->mesh = std::make_shared<gpu::MeshPN>();
            piece->mesh->setMeshData(*meshData);
            piece->mesh->build(device);
            piece->model = model;

            InstanceData& inst = piece->instance;
            inst.albedo = glm::vec4(albedo, roughness);
            inst.emission = glm::vec4(emission, static_cast<float>(shapeType));
            inst.extents = glm::vec4(0.0f);

            const glm::vec3 center = glm::vec3(model[3]);
            switch (shapeType)
            {
            case ShapeType::Plane:
                // Plane local normal is +Y; store the world-space normal.
                inst.normalOrCenter =
                    glm::vec4(glm::normalize(glm::mat3(model) * glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f);
                break;
            case ShapeType::Sphere:
                inst.normalOrCenter = glm::vec4(center, shapeParam.w);
                break;
            case ShapeType::Cube:
                inst.normalOrCenter = glm::vec4(center, 0.0f);
                inst.extents = glm::vec4(glm::vec3(shapeParam), 0.0f);
                break;
            }

            for (uint32_t s = 0; s < piece->mesh->submeshCount(); ++s)
            {
                auto rtMesh = device->createRayTracingMesh(piece->mesh->rayTracingMeshDescription(s));
                _cleanup->push(rtMesh);
                piece->rtMeshes.push_back(std::move(rtMesh));
            }

        };

        constexpr float kWallRoughness = 0.5f; // matte-ish walls (rough at 50%)
        constexpr float kShiny = 0.0f;         // mirror-like objects

        // Floor (matte grey)
        addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
                 glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

        // Ceiling (matte grey)
        addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
                 glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.5f, 0.0f)),
                             glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

        // Back wall (matte grey)
        addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
                 glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.75f, -2.75f)),
                             glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

        // Left wall (matte red)
        addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
                 glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 2.75f, 0.0f)),
                             glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.63f, 0.06f, 0.06f), glm::vec3(0.0f));

        // Right wall (matte green)
        addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
                 glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(2.75f, 2.75f, 0.0f)),
                             glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.12f, 0.45f, 0.15f), glm::vec3(0.0f));

        // Light rectangle at ceiling (emissive, black albedo so paths terminate on it)
        addPiece(bg2e::geo::createPlanePN(1.3f, 1.3f),
                 glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.49f, 0.0f)),
                 ShapeType::Plane, kWallRoughness,
                 glm::vec3(0.0f), glm::vec3(18.0f, 18.0f, 18.0f));

        // Cube (shiny / mirror-like)
        addPiece(bg2e::geo::createCubePN(1.2f, 1.2f, 1.2f),
                 glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.6f, 0.5f)),
                 ShapeType::Cube, kShiny,
                 glm::vec3(0.95f, 0.95f, 0.95f), glm::vec3(0.0f),
                 glm::vec4(0.6f, 0.6f, 0.6f, 0.0f));

        // Sphere (shiny / mirror-like)
        addPiece(bg2e::geo::createSpherePN(0.6f, 32, 24),
                 glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.6f, -0.5f)),
                 ShapeType::Sphere, kShiny,
                 glm::vec3(0.95f, 0.95f, 0.95f), glm::vec3(0.0f),
                 glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));

        device->immediateSubmit([&](gpu::CommandBuffer* cmd)
                                {
                                    for (auto& piece : _pieces)
                                    {
                                        for (auto& rtMesh : piece->rtMeshes)
                                        {
                                            cmd->buildRayTracingMesh(rtMesh.get());
                                        }
                                    }
                                });

        auto rayTracingScene = device->createRayTracingScene("Cornell box scene");
        _cleanup->push(rayTracingScene);

        // The instance metadata array is built in lock-step with addInstance so
        // that gl_InstanceCustomIndexEXT (Vulkan) / intersection.instance_id
        // (Metal) directly indexes the matching InstanceData entry.
        uint32_t instanceId = 0;
        for (auto& piece : _pieces)
        {
            for (auto& rtMesh : piece->rtMeshes)
            {
                rayTracingScene->addInstance(rtMesh.get(), piece->model, instanceId++, 0xFF);
                allInstanceData.push_back(piece->instance);
            }
        }

        device->immediateSubmit([&](gpu::CommandBuffer* cmd)
                                {
                                    rayTracingScene->buildOrUpdate(cmd);
                                });

        auto instanceBuffer = device->createBuffer("Instance Data SSBO");
        _cleanup->push(instanceBuffer);
        instanceBuffer->createStorageBuffer(allInstanceData.data(),
                                            allInstanceData.size() * sizeof(InstanceData));

        _sceneSet = device->createResourceSet(_rtLayout.get(), 1, "Cornell box scene set");
        _cleanup->push(_sceneSet);
        _sceneSet->setRayTracingScene({.vulkan = 0, .metal = 2}, rayTracingScene.get());
        _sceneSet->update();
        _dataSet = device->createResourceSet(_rtLayout.get(), 2, "Instance data set");
        _cleanup->push(_dataSet);
        _dataSet->setStorageBuffer({.vulkan = 0, .metal = 3}, instanceBuffer.get());
        _dataSet->update();
        gpu::RayTracingPipelineDescription rt{};
        rt.raygenShader = rgen.get();
        rt.missShader = rmiss.get();
        rt.closestHitShader = rchit.get();
        rt.layout = _rtLayout.get();
        rt.maxRecursionDepth = 1;
        rt.debugName = "Cornell box RT pipeline";
        _rtPipeline = device->createRayTracingPipeline(rt);
        _cleanup->push(_rtPipeline);

        _outputVs = shaders.vertex("output", device);
        _cleanup->push(_outputVs);
        _outputFs = shaders.fragment("output", device);
        _cleanup->push(_outputFs);
        gpu::PipelineLayoutDescription outputLayout{};
        outputLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        outputLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        outputLayout.debugName = "Fullscreen RT output layout";
        _outputLayout = device->createPipelineLayout(outputLayout);
        _cleanup->push(_outputLayout);
        _sampler = device->createSampler({.addressModeU = gpu::AddressMode::ClampToEdge,
                                         .addressModeV = gpu::AddressMode::ClampToEdge,
                                         .debugName = "RT output sampler"});
        _cleanup->push(_sampler);
    }

    void resize(gpu::Size2D extent) override
    {
        // draw drains GPU work before replacing size-dependent resources.
        cleanupSizeResources();
        auto* device = _engine->device();
        auto* surface = _engine->surface();
        _outputImage = device->createImage({.size = extent,
                                           .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                           .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled,
                                           .debugName = "RT sampled output"});
        _accumImage = device->createImage({.size = extent,
                                          .format = gpu::PixelFormat::R32G32B32A32_SFLOAT,
                                          .usage = gpu::ImageUsage::Storage,
                                          .debugName = "RT HDR accumulation"});
        const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        _camera.viewInverse = glm::inverse(glm::lookAt(glm::vec3(0.0f, 2.7f, 15.0f),
                                                       glm::vec3(0.0f, 2.7f, 0.0f),
                                                       glm::vec3(0.0f, 1.0f, 0.0f)));
        _camera.projInverse = glm::inverse(glm::perspective(glm::radians(40.0f), aspect, 0.1f, 100.0f));
        _camera.misc = glm::vec4(0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f);
        _controller->samples = 0;
        _controller->resetRequested = false;
        _cameraRing.create(surface, [this, device](uint32_t slot)
        {
            auto buffer = device->createBuffer("RT camera slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(_camera);
            return buffer;
        });
        _rtOutputRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_rtLayout.get(), 0, "RT output slot[" + std::to_string(slot) + "]");
            set->setStorageImage({.vulkan = 0, .metal = 0}, _outputImage.get());
            set->setUniformBuffer({.vulkan = 1, .metal = 1}, _cameraRing.sharedAt(slot));
            set->setStorageImage({.vulkan = 2, .metal = 1}, _accumImage.get());
            set->update();
            return set;
        });
        _outputSet = device->createResourceSet(_outputLayout.get(), 0, "Fullscreen RT image set");
        _outputSet->setSampledImage({.vulkan = 0, .metal = 0}, _outputImage.get());
        _outputSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
        _outputSet->update();
        gpu::GraphicsPipelineDescription output{};
        output.vertexShader = _outputVs.get();
        output.fragmentShader = _outputFs.get();
        output.layout = _outputLayout.get();
        output.colorFormat = surface->colorFormat();
        output.depthFormat = gpu::PixelFormat::Undefined;
        output.cullMode = gpu::CullMode::None;
        output.debugName = "Fullscreen RT presentation";
        _outputPipeline = device->createGraphicsPipeline(output);
    }

    void update(const draw::FrameContext& context) override
    {
        if (_controller->resetRequested)
        {
            _controller->samples = 0;
            _controller->resetRequested = false;
        }
        ++_controller->samples;
        _camera.misc.x = static_cast<float>(_controller->samples);
        const auto& color = _controller->color;
        _camera.background = glm::vec4(color.r, color.g, color.b, color.a);
        _cameraRing.at(context.frameSlot)->updateUniformBuffer(_camera);
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        // The shared running mean is ordered across frames on draw's graphics queue.
        // General -> General also provides a dependency for accumulation writes.
        command.transition(_outputImage.get(), gpu::ImageLayout::General);
        command.transition(_accumImage.get(), gpu::ImageLayout::General);
        command.bindPipeline(_rtPipeline.get());
        command.bindResourceSet(_rtPipeline.get(), 0, _rtOutputRing.at(context.frameSlot));
        command.bindResourceSet(_rtPipeline.get(), 1, _sceneSet.get());
        command.bindResourceSet(_rtPipeline.get(), 2, _dataSet.get());
        command.traceRays(context.extent.width, context.extent.height, 1);
        // Metal represents RT dispatch as a compute scope; close it before graphics.
        if (_engine->backendType() == gpu::BackendType::Metal)
        {
            command.endCompute();
        }
        command.transition(_outputImage.get(), gpu::ImageLayout::ShaderReadOnly);
        command.beginRendering(&context.colorTarget);
        command.bindPipeline(_outputPipeline.get());
        command.bindResourceSet(_outputPipeline.get(), 0, _outputSet.get());
        command.draw(3);
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        cleanupSizeResources();
        if (_cleanup)
        {
            // Descriptor sets -> TLAS -> BLAS, while borrowed mesh buffers remain alive.
            _cleanup->flush();
            _cleanup.reset();
        }
        for (auto& piece : _pieces)
        {
            piece->rtMeshes.clear();
            if (piece->mesh)
            {
                piece->mesh->cleanup();
            }
        }
        _pieces.clear();
        _sceneSet.reset();
        _dataSet.reset();
        _rtPipeline.reset();
        _rtLayout.reset();
        _sampler.reset();
        _outputLayout.reset();
        _outputFs.reset();
        _outputVs.reset();
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

    void cleanupSizeResources()
    {
        release(_outputPipeline);
        release(_outputSet);
        _rtOutputRing.cleanup();
        _rtOutputRing.clear();
        _cameraRing.cleanup();
        _cameraRing.clear();
        release(_accumImage);
        release(_outputImage);
    }

    std::shared_ptr<Controller> _controller;
    CameraData _camera;
    std::vector<std::shared_ptr<ScenePiece>> _pieces;
    std::unique_ptr<gpu::CleanupManager> _cleanup;
    gpu::FrameResourceRing<gpu::Buffer> _cameraRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _rtOutputRing;
    std::shared_ptr<gpu::PipelineLayout> _rtLayout;
    std::shared_ptr<gpu::RayTracingPipeline> _rtPipeline;
    std::shared_ptr<gpu::ResourceSet> _sceneSet;
    std::shared_ptr<gpu::ResourceSet> _dataSet;
    std::shared_ptr<gpu::Image> _outputImage;
    std::shared_ptr<gpu::Image> _accumImage;
    std::shared_ptr<gpu::ShaderModule> _outputVs;
    std::shared_ptr<gpu::ShaderModule> _outputFs;
    std::shared_ptr<gpu::PipelineLayout> _outputLayout;
    std::shared_ptr<gpu::GraphicsPipeline> _outputPipeline;
    std::shared_ptr<gpu::ResourceSet> _outputSet;
    std::shared_ptr<gpu::Sampler> _sampler;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Path tracing controls");
        _window.setPosition(20, 20);
        _window.setSize(440, 360);
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
             ui::Text::text("Device: " + state.deviceName);
             ui::Text::text("Accumulated samples per pixel: " + std::to_string(state.samples));
             if (ui::Button::button("Restart accumulation"))
             {
                 state.resetRequested = true;
                 state.loop.requestSceneFrame();
             }
             ui::Text::text("UI frames: " + std::to_string(state.uiFrames));
             ui::Text::text("Scene updates: " + std::to_string(state.sceneFrames));
             if (ui::Value::colorPicker("Background", state.color))
             {
                 state.resetRequested = true;
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.ray-tracing-pipeline");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: ray tracing pipeline", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_ray_tracing_pipeline: " << error.what() << '\n';
        return 1;
    }
}
