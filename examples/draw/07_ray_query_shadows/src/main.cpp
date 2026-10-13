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
#include <bg2e/math/base.hpp>
#include <bg2e/gpu/Mesh.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Window.hpp>

#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace bg2e;

// Shared application state is accessed exclusively on the main thread.
struct Controller {
    app::MainLoop& loop;
    base::Color color{0.04f, 0.05f, 0.07f, 1.0f};
    bool paused = false;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
    std::string deviceName;
};

struct CameraUBO {
    glm::mat4 projectionView{1.0f};
};

struct ObjectUBO {
    glm::mat4 model{1.0f};
    glm::vec4 albedo{1.0f};
};

struct LightUBO {
    glm::vec4 lightPosition{0.0f};
    glm::vec4 lightColor{0.0f};
    glm::vec4 ambient{0.0f};
};

struct SceneObject {
    gpu::MeshPN mesh;
    glm::mat4 model{1.0f};
    std::shared_ptr<gpu::Buffer> objectUbo;
    std::shared_ptr<gpu::ResourceSet> objectSet;
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
            std::string message = "This example requires hardware accelerated ray queries. Device: " + properties->name;
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
        gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_ray_query_shadows",
                               _engine->backendType());
        _vertexShader = shaders.vertex("shadow", device);
        _cleanup->push(_vertexShader);
        _fragmentShader = shaders.fragment("shadow", device);
        _cleanup->push(_fragmentShader);

        gpu::PipelineLayoutDescription layout{};
        layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        layout.resourceBindings.push_back({1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        layout.resourceBindings.push_back({2, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({2, {.vulkan = 1, .metal = 2}, gpu::ResourceType::AccelerationStructure, gpu::ShaderStage::Fragment, 1});
        layout.debugName = "Ray query shadows pipeline layout";
        _layout = device->createPipelineLayout(layout);
        _cleanup->push(_layout);

        _cameraUbo = device->createBuffer("Camera UBO");
        _cleanup->push(_cameraUbo);
        _cameraUbo->createUniformBuffer(CameraUBO{});
        _cameraSet = device->createResourceSet(_layout.get(), 0, "Camera resource set");
        _cleanup->push(_cameraSet);
        _cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, _cameraUbo);
        _cameraSet->update();

        // Keep the raster mesh alive until every referencing BLAS is cleaned.
        auto addObject = [this, device](geo::MeshPN* data, const glm::mat4& model, const glm::vec4& albedo)
        {
            std::unique_ptr<geo::MeshPN> meshData(data);
            auto object = std::make_unique<SceneObject>();
            auto& obj = *object;
            _objects.push_back(std::move(object));
            obj.mesh.setMeshData(*meshData);
            obj.mesh.build(device);
            obj.model = model;
            obj.objectUbo = device->createBuffer("Object UBO");
            _cleanup->push(obj.objectUbo);
            obj.objectUbo->createUniformBuffer(ObjectUBO{model, albedo});
            obj.objectSet = device->createResourceSet(_layout.get(), 1, "Object resource set");
            _cleanup->push(obj.objectSet);
            obj.objectSet->setUniformBuffer({.vulkan = 0, .metal = 3}, obj.objectUbo);
            obj.objectSet->update();
            for (uint32_t submesh = 0; submesh < obj.mesh.submeshCount(); ++submesh)
            {
                auto rtMesh = device->createRayTracingMesh(obj.mesh.rayTracingMeshDescription(submesh));
                _cleanup->push(rtMesh);
                obj.rtMeshes.push_back(std::move(rtMesh));
            }
        };
        addObject(geo::createPlanePN(12.0f, 12.0f), glm::mat4(1.0f),
                  glm::vec4(0.75f, 0.75f, 0.78f, 1.0f));
        addObject(geo::createCubePN(1.2f, 1.2f, 1.2f),
                  glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.6f, 0.4f)),
                  glm::vec4(0.85f, 0.25f, 0.20f, 1.0f));
        addObject(geo::createSpherePN(0.75f, 32, 24),
                  glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.75f, -0.3f)),
                  glm::vec4(0.20f, 0.45f, 0.85f, 1.0f));
        device->immediateSubmit([this](gpu::CommandBuffer* command)
        {
            for (auto& obj : _objects)
            {
                for (auto& rtMesh : obj->rtMeshes)
                {
                    command->buildRayTracingMesh(rtMesh.get());
                }
            }
        });
    }

    void resize(gpu::Size2D extent) override
    {
        // draw has drained GPU work, including all previous ring slots.
        auto* device = _engine->device();
        auto* surface = _engine->surface();
        release(_pipeline);
        release(_depth);
        cleanupFrameResources();

        const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        const auto projection = glm::perspective(glm::radians(55.0f), aspect, 0.1f, 100.0f);
        const auto view = glm::lookAt(glm::vec3(4.5f, 4.0f, 5.5f),
                                      glm::vec3(0.0f, 0.6f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
        _cameraUbo->updateUniformBuffer(CameraUBO{projection * view});
        _depth = device->createImage({.size = extent,
                                     .format = surface->depthFormat(),
                                     .usage = gpu::ImageUsage::DepthStencil,
                                     .debugName = "Ray query shadows depth"});

        // A scene owns mutable instance, scratch, and acceleration buffers.
        // Duplicate it per slot as well as the moving light's uniform buffer.
        _sceneRing.create(surface, [this, device](uint32_t slot)
        {
            auto scene = device->createRayTracingScene("Shadow scene slot[" + std::to_string(slot) + "]");
            populateInstances(scene.get());
            return scene;
        });
        // All TLAS handles must exist before creating acceleration descriptors.
        device->immediateSubmit([this](gpu::CommandBuffer* command)
        {
            for (uint32_t slot = 0; slot < _sceneRing.size(); ++slot)
            {
                _sceneRing.at(slot)->buildOrUpdate(command);
            }
        });
        _lightUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Light UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(LightUBO{});
            return buffer;
        });
        _lightSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_layout.get(), 2,
                                                 "Light resource set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 1}, _lightUboRing.sharedAt(slot));
            set->setRayTracingScene({.vulkan = 1, .metal = 2}, _sceneRing.at(slot));
            set->update();
            return set;
        });

        gpu::GraphicsPipelineDescription pipeline{};
        pipeline.vertexShader = _vertexShader.get();
        pipeline.fragmentShader = _fragmentShader.get();
        pipeline.layout = _layout.get();
        pipeline.topology = gpu::PrimitiveTopology::TriangleList;
        pipeline.colorFormat = surface->colorFormat();
        pipeline.depthFormat = _depth->pixelFormat();
        pipeline.cullMode = gpu::CullMode::None;
        pipeline.debugName = "Ray query shadows pipeline";
        pipeline.addVertexBufferDescription(gpu::MeshPN::vertexBufferDescription());
        _pipeline = device->createGraphicsPipeline(pipeline);
    }

    void update(const draw::FrameContext& context) override
    {
        _time += context.deltaSeconds;
        LightUBO light{};
        const float orbitRadius = 3.2f;
        light.lightPosition = glm::vec4(orbitRadius * std::cos(_time), 4.0f,
                                         orbitRadius * std::sin(_time), 1.0f);
        light.lightColor = glm::vec4(1.0f, 0.97f, 0.9f, 1.6f);
        light.ambient = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f);
        // beginFrame has waited for this slot before update/render are called.
        _lightUboRing.at(context.frameSlot)->updateUniformBuffer(light);
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        auto* scene = _sceneRing.at(context.frameSlot);
        // Keep the source example's dynamic TLAS rebuild demonstration.
        // Instance count stays constant, preserving the bound native handle.
        populateInstances(scene);
        scene->buildOrUpdate(&command);

        command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
        command.beginRendering(&context.colorTarget, _depth.get());
        const auto& color = _controller->color;
        command.clearColor(0, {color.r, color.g, color.b, color.a});
        command.clearDepth(1.0f);
        command.bindPipeline(_pipeline.get());
        command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
        command.bindResourceSet(_pipeline.get(), 2, _lightSetRing.at(context.frameSlot));
        for (auto& obj : _objects)
        {
            command.bindResourceSet(_pipeline.get(), 1, obj->objectSet.get());
            obj->mesh.draw(&command);
        }
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        release(_pipeline);
        release(_depth);
        cleanupFrameResources();
        if (_cleanup)
        {
            // TLAS rings are gone. Clean BLAS before their borrowed mesh buffers.
            _cleanup->flush();
            _cleanup.reset();
        }
        for (auto& obj : _objects)
        {
            obj->rtMeshes.clear();
            obj->objectSet.reset();
            obj->objectUbo.reset();
            obj->mesh.cleanup();
        }
        _objects.clear();
        _cameraSet.reset();
        _cameraUbo.reset();
        _layout.reset();
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

    void populateInstances(gpu::RayTracingScene* scene)
    {
        scene->clearInstances();
        uint32_t instanceId = 0;
        for (auto& obj : _objects)
        {
            for (auto& rtMesh : obj->rtMeshes)
            {
                scene->addInstance(rtMesh.get(), obj->model, instanceId++, 0xFF);
            }
        }
    }

    void cleanupFrameResources()
    {
        _lightSetRing.cleanup();
        _lightSetRing.clear();
        _lightUboRing.cleanup();
        _lightUboRing.clear();
        _sceneRing.cleanup();
        _sceneRing.clear();
    }

    std::shared_ptr<Controller> _controller;
    float _time = 0.0f;
    std::vector<std::unique_ptr<SceneObject>> _objects;
    gpu::FrameResourceRing<gpu::RayTracingScene> _sceneRing;
    gpu::FrameResourceRing<gpu::Buffer> _lightUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _lightSetRing;
    std::unique_ptr<gpu::CleanupManager> _cleanup;
    std::shared_ptr<gpu::ShaderModule> _vertexShader;
    std::shared_ptr<gpu::ShaderModule> _fragmentShader;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::Image> _depth;
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
        _window.setTitle("Ray query shadow controls");
        _window.setPosition(20, 20);
        _window.setSize(440, 340);
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
             ui::Text::text("Fragment ray queries: enabled");
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.ray-query-shadows");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: ray query shadows", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_ray_query_shadows: " << error.what() << '\n';
        return 1;
    }
}
