#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/environment/all.hpp>
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
#include <vector>
#include <cstddef>

namespace {
using namespace bg2e;

// Shared application state is accessed exclusively on the main thread.
struct Controller {
    app::MainLoop& loop;
    base::Color color{0.12f, 0.18f, 0.28f, 1.0f};
    bool paused = false;
    uint32_t activeMap = 0;
    uint32_t specularLod = 0;
    uint32_t specularMipLevels = 1;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
};

struct CameraUBO {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 cameraPos;
};

struct ModelUBO {
    glm::mat4 model{1.0f};
};

struct ReflectPushConstants {
    glm::vec3 cameraPos;
    float lod = 0.0f;
};

static_assert(sizeof(ReflectPushConstants) == 16);
static_assert(offsetof(ReflectPushConstants, lod) == 12);

const std::vector<std::string> mapNames = {"Original environment", "Diffuse irradiance", "Specular reflection"};

class SceneDelegate final : public draw::RenderLoopDelegate {
  public:
    explicit SceneDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void initScene() override
    {
        auto* device = _engine->device();
        _cleanup = std::make_unique<gpu::CleanupManager>(_engine->surface());
        const auto shaderRoot = base::PlatformTools::shaderPath();
        gpu::ShaderLib shaderLib(shaderRoot / "draw_cubemap_render_pass", _engine->backendType());
        auto reflectVs = shaderLib.vertex("reflect_cube", device);
        auto reflectFs = shaderLib.fragment("reflect_cube", device);
        _cleanup->push(reflectVs);
        _cleanup->push(reflectFs);
        gpu::PipelineLayoutDescription reflectLayoutDesc{};
        reflectLayoutDesc.pushConstants.push_back(
            {0, sizeof(ReflectPushConstants), gpu::ShaderStage::Fragment});
        reflectLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        reflectLayoutDesc.resourceBindings.push_back(
            {1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        reflectLayoutDesc.resourceBindings.push_back(
            {2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        reflectLayoutDesc.resourceBindings.push_back(
            {2, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        reflectLayoutDesc.debugName = "Reflect cube pipeline layout";
        auto reflectLayout = device->createPipelineLayout(reflectLayoutDesc);
        _cleanup->push(reflectLayout);


        _environmentGenerator = std::make_unique<draw::environment::EnvironmentGenerator>(*_engine, shaderRoot);
        _environmentResources = _environmentGenerator->build(base::PlatformTools::assetPath() / "autumn_field_4k.hdr");
        _controller->specularMipLevels = _environmentResources->prefilteredSpecular->mipLevels();
        _skybox.settings().shaderRoot = shaderRoot;
        _skybox.setEnvironment(_environmentResources);
        // On-screen reflective cube.
        std::unique_ptr<bg2e::geo::MeshPNU> reflectCubeData(bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f));
        _cube.setMeshData(*reflectCubeData);
        _cube.build(device);


        _reflectVs = reflectVs;
        _reflectFs = reflectFs;
        _reflectLayout = reflectLayout;
        _cameraUbo = device->createBuffer("Camera UBO");
        _cleanup->push(_cameraUbo);
        _cameraUbo->createUniformBuffer(CameraUBO{});
        _cameraSet = device->createResourceSet(_reflectLayout.get(), 0, "Camera resource set");
        _cleanup->push(_cameraSet);
        _cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, _cameraUbo);
        _cameraSet->update();


        auto makeCubeSet = [this, device](const std::shared_ptr<gpu::Image>& map, const std::string& name)
        {
            auto set = device->createResourceSet(_reflectLayout.get(), 2, name);
            _cleanup->push(set);
            set->setSampledImage({.vulkan = 0, .metal = 0}, map.get());
            set->setSampler({.vulkan = 1, .metal = 0}, _environmentResources->sampler.get());
            set->update();
            return set;
        };
        _cubeSets = {
            makeCubeSet(_environmentResources->sky, "Reflect original cube set"),
            makeCubeSet(_environmentResources->irradiance, "Reflect irradiance cube set"),
            makeCubeSet(_environmentResources->prefilteredSpecular, "Reflect specular cube set")
        };
    }

    void resize(gpu::Size2D extent) override
    {
        // Generated cubemaps keep their fixed resolutions across window resize.
        auto* device = _engine->device();
        auto* surface = _engine->surface();
        release(_pipeline);
        release(_depth);
        cleanupModelResources();
        CameraUBO camera{};
        camera.projection = glm::perspective(glm::radians(60.0f),
                                             static_cast<float>(extent.width) / static_cast<float>(extent.height),
                                             0.1f, 1000.0f);
        camera.view = glm::lookAt(_cameraEye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        camera.cameraPos = glm::vec4(_cameraEye, 1.0f);
        _cameraUbo->updateUniformBuffer(camera);
        _depth = device->createImage({.size = extent,
                                     .format = surface->depthFormat(),
                                     .usage = gpu::ImageUsage::DepthStencil,
                                     .debugName = "Reflective cube depth"});
        _modelUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Model UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(ModelUBO{});
            return buffer;
        });
        _modelSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_reflectLayout.get(), 1,
                                                 "Model set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 3}, _modelUboRing.sharedAt(slot));
            set->update();
            return set;
        });
        gpu::GraphicsPipelineDescription pipeline{};
        pipeline.vertexShader = _reflectVs.get();
        pipeline.fragmentShader = _reflectFs.get();
        pipeline.layout = _reflectLayout.get();
        pipeline.colorFormat = surface->colorFormat();
        pipeline.depthFormat = _depth->pixelFormat();
        pipeline.debugName = "Reflect cube pipeline";
        pipeline.addVertexBufferDescription(gpu::MeshPNU::vertexBufferDescription());
        _pipeline = device->createGraphicsPipeline(pipeline);
        _skybox.initialize(*_engine, surface->colorFormat(), _depth->pixelFormat());
        _skybox.setCamera(camera.view, camera.projection);
    }

    void update(const draw::FrameContext& context) override
    {
        _time += context.deltaSeconds;
        ModelUBO model{};
        model.model = glm::rotate(glm::mat4(1.0f), _time,
                                  glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f)));
        _modelUboRing.at(context.frameSlot)->updateUniformBuffer(model);
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
        command.beginRendering(&context.colorTarget, _depth.get());
        const auto& color = _controller->color;
        command.clearColor(0, {color.r, color.g, color.b, color.a});
        command.clearDepth(1.0f);
        _skybox.draw(command, context.extent);
        ReflectPushConstants push{};
        push.cameraPos = _cameraEye;
        push.lod = _controller->activeMap == 2 ? static_cast<float>(_controller->specularLod) : 0.0f;
        command.bindPipeline(_pipeline.get());
        command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
        command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
        command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
        command.bindResourceSet(_pipeline.get(), 2, _cubeSets[_controller->activeMap].get());
        _cube.draw(&command);
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        release(_pipeline);
        release(_depth);
        cleanupModelResources();
        _cube.cleanup();
        if (_cleanup)
        {
            _cleanup->flush();
            _cleanup.reset();
        }
        for (auto& set : _cubeSets)
        {
            set.reset();
        }
        _cameraSet.reset();
        _cameraUbo.reset();
        _reflectLayout.reset();
        _reflectFs.reset();
        _reflectVs.reset();
        _skybox.cleanup();
        _environmentResources.reset();
        if (_environmentGenerator)
        {
            _environmentGenerator->cleanup();
            _environmentGenerator.reset();
        }
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

    void cleanupModelResources()
    {
        _modelSetRing.cleanup();
        _modelSetRing.clear();
        _modelUboRing.cleanup();
        _modelUboRing.clear();
    }

    std::unique_ptr<draw::environment::EnvironmentGenerator> _environmentGenerator;
    std::shared_ptr<const draw::environment::EnvironmentResources> _environmentResources;
    draw::environment::SkyboxRenderer _skybox;
    std::shared_ptr<Controller> _controller;
    float _time = 0.0f;
    const glm::vec3 _cameraEye{0.0f, 0.0f, 3.0f};
    gpu::MeshPNU _cube;
    gpu::FrameResourceRing<gpu::Buffer> _modelUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _modelSetRing;
    std::unique_ptr<gpu::CleanupManager> _cleanup;
    std::shared_ptr<gpu::ShaderModule> _reflectVs;
    std::shared_ptr<gpu::ShaderModule> _reflectFs;
    std::shared_ptr<gpu::PipelineLayout> _reflectLayout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::Image> _depth;
    std::shared_ptr<gpu::Buffer> _cameraUbo;
    std::shared_ptr<gpu::ResourceSet> _cameraSet;
    std::array<std::shared_ptr<gpu::ResourceSet>, 3> _cubeSets;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Cubemap render pass controls");
        _window.setPosition(20, 20);
        _window.setSize(440, 390);
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
             if (ui::Value::comboBox("Reflection map", mapNames, state.activeMap))
             {
                 state.loop.requestSceneFrame();
             }
             ui::Text::text("Specular mip: " + std::to_string(state.specularLod) +
                            " / " + std::to_string(state.specularMipLevels - 1));
             if (ui::Button::button("Next specular mip", false, state.activeMap != 2))
             {
                 state.specularLod = (state.specularLod + 1) % state.specularMipLevels;
                 state.loop.requestSceneFrame();
             }
             ui::Text::text("Space: next map. Enter: next specular mip.");
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

class InputDelegate final : public app::InputDelegate {
  public:
    explicit InputDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void keyUp(const app::KeyEvent& event) override
    {
        if (event.key() == app::KeyEvent::KeySpace)
        {
            _controller->activeMap = (_controller->activeMap + 1) % 3;
            _controller->loop.requestSceneFrame();
        }
        else if (event.key() == app::KeyEvent::KeyEnter)
        {
            _controller->specularLod = (_controller->specularLod + 1) % _controller->specularMipLevels;
            _controller->loop.requestSceneFrame();
        }
    }

  private:
    std::shared_ptr<Controller> _controller;
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
        setInputDelegate(std::make_shared<InputDelegate>(_controller));
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.cubemap-render-pass");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: cubemap render pass", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_cubemap_render_pass: " << error.what() << '\n';
        return 1;
    }
}
