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
#include <bg2e/db/image.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/geo/sphere.hpp>
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
    bool debugCubemapMode = false;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
};

struct CubemapCameraPushConstants {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
};

struct CubeReflectionPushConstants {
    glm::vec3 cameraPos;
    float padding = 0.0f;
};

struct ModelUBO {
    glm::mat4 model{1.0f};
};

struct CameraUBO {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 cameraPos{0.0f};
};

struct CubeRenderSettingsUBO {
    uint32_t mode = 0; // 0 = reflection, 1 = direct cubemap debug
    uint32_t _pad0 = 0;
    uint32_t _pad1 = 0;
    uint32_t _pad2 = 0;
};

struct CubemapFaceCamera {
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;
};

const std::array<CubemapFaceCamera, 6> faceCameras = {{
    {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},  // +X
    {glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)}, // -X
    {glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, // +Y
    {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},   // -Y
    {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},  // +Z
    {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}, // -Z
}};

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
        gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_cubemap", _engine->backendType());
        _cubemapRendererVs = shaders.vertex("cubemap_renderer", device);
        _cleanup->push(_cubemapRendererVs);
        _cubemapRendererFs = shaders.fragment("cubemap_renderer", device);
        _cleanup->push(_cubemapRendererFs);
        _cubeReflectionVs = shaders.vertex("reflect_cube", device);
        _cleanup->push(_cubeReflectionVs);
        _cubeReflectionFs = shaders.fragment("reflect_cube", device);
        _cleanup->push(_cubeReflectionFs);
        // Cubemap renderer pipeline layout
        //   push constants (vertex): projection + view matrices
        //   set 0 binding 0: model UBO (vertex)
        //   set 1 binding 0: equirect texture (fragment)
        //   set 1 binding 1: sampler (fragment)
        gpu::PipelineLayoutDescription cubemapRendererLayoutDesc{};
        cubemapRendererLayoutDesc.pushConstants.push_back(
            // Vertex push constants use buffer(1) on Metal.
            {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex});
        cubemapRendererLayoutDesc.resourceBindings.push_back({0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        cubemapRendererLayoutDesc.resourceBindings.push_back({1, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        cubemapRendererLayoutDesc.resourceBindings.push_back({1, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        cubemapRendererLayoutDesc.debugName = "Cubemap renderer pipeline layout";
        _cubemapRendererLayout = device->createPipelineLayout(cubemapRendererLayoutDesc);
        _cleanup->push(_cubemapRendererLayout);

        // Cube reflection pipeline layout
        //   push constants (fragment): cameraPos
        //   vk = set 0 binding 0 / metal = binding 2: camera UBO (vertex)
        //   vk = set 1 binding 0 / metal = binding 3: model UBO (vertex)
        //   vk = set 2 binding 0 / metal = binding 0: cubemap texture (fragment)
        //   vk = set 2 binding 1 / metal = binding 0: sampler (fragment)
        //   vk = set 3 binding 0 / metal = binding 3: render settings UBO (fragment)
        gpu::PipelineLayoutDescription cubeReflectionLayoutDesc{};
        cubeReflectionLayoutDesc.pushConstants.push_back(
            // Fragment push constants use buffer(0) on Metal.
            {0, sizeof(CubeReflectionPushConstants), gpu::ShaderStage::Fragment});
        cubeReflectionLayoutDesc.resourceBindings.push_back({0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        cubeReflectionLayoutDesc.resourceBindings.push_back({1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
        cubeReflectionLayoutDesc.resourceBindings.push_back({2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        cubeReflectionLayoutDesc.resourceBindings.push_back({2, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        cubeReflectionLayoutDesc.resourceBindings.push_back({3, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
        cubeReflectionLayoutDesc.debugName = "Cube reflection pipeline layout";
        _cubeReflectionLayout = device->createPipelineLayout(cubeReflectionLayoutDesc);
        _cleanup->push(_cubeReflectionLayout);

        // Equirectangular HDR texture
        auto assetsPath = base::PlatformTools::assetPath();
        auto imageData = std::shared_ptr<base::Image>(db::loadImage(assetsPath / "autumn_field_4k.hdr"));
        if (!imageData || imageData->width() == 0 || imageData->height() == 0)
        {
            throw std::runtime_error("Failed to load the cubemap HDR environment.");
        }
        _equirectTexture = device->createImage({.size = {imageData->width(), imageData->height()},
                                                    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                                    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                                    .debugName = "Equirect HDR texture"});
        _cleanup->push(_equirectTexture);
        _equirectTexture->uploadImage(imageData.get());

        device->immediateSubmit([this](gpu::CommandBuffer* cmd)
                                {
                                    cmd->transition(_equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
                                });
        _sampler = device->createSampler({.debugName = "Default linear sampler"});
        _cleanup->push(_sampler);

        // Equirect texture resource set (set 1): texture + sampler, persistent.
        _equirectTextureSet = device->createResourceSet(_cubemapRendererLayout.get(), 1, "Equirect texture set");
        _equirectTextureSet->setSampledImage({.vulkan = 0, .metal = 0}, _equirectTexture.get());
        _equirectTextureSet->setSampler({.vulkan = 1, .metal = 0}, _sampler.get());
        _equirectTextureSet->update();
        _cleanup->push(_equirectTextureSet);

        // Cubemap render target
        const uint32_t cubemapSize = 512;
        _cubeMap = device->createCubeMap({.size = cubemapSize,
                                              .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                              .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
                                              .mipLevels = 1,
                                              .debugName = "Skybox cubemap"});
        _cleanup->push(_cubeMap);

        // Cubemap texture resource set (set 2): cubemap image + sampler, persistent.
        _cubemapTextureSet = device->createResourceSet(_cubeReflectionLayout.get(), 2, "Cubemap texture set");
        _cubemapTextureSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, _cubeMap.get());
        _cubemapTextureSet->setSampler({.vulkan = 1, .metal = 0}, _sampler.get());
        _cubemapTextureSet->update();
        _cleanup->push(_cubemapTextureSet);

        _cameraUbo = device->createBuffer("Camera UBO");
        _cleanup->push(_cameraUbo);
        _cameraUbo->createUniformBuffer(CameraUBO{});
        _cameraSet = device->createResourceSet(_cubeReflectionLayout.get(), 0, "Camera resource set");
        _cleanup->push(_cameraSet);
        _cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, _cameraUbo);
        _cameraSet->update();
        std::unique_ptr<geo::MeshPU> sphereData(geo::createSpherePU(50.0f, 16, 16, true));
        _cubemapSphere.setMeshData(*sphereData);
        _cubemapSphere.build(device);
        std::unique_ptr<geo::MeshPNU> cubeData(geo::createCubePNU(1.0f, 1.0f, 1.0f));
        _cube.setMeshData(*cubeData);
        _cube.build(device);
        // Cubemap renderer pipeline
        gpu::GraphicsPipelineDescription cubemapRendererPipelineDesc{};
        cubemapRendererPipelineDesc.vertexShader = _cubemapRendererVs.get();
        cubemapRendererPipelineDesc.fragmentShader = _cubemapRendererFs.get();
        cubemapRendererPipelineDesc.layout = _cubemapRendererLayout.get();
        cubemapRendererPipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
        cubemapRendererPipelineDesc.colorFormat = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        cubemapRendererPipelineDesc.depthFormat = gpu::PixelFormat::Undefined;
        cubemapRendererPipelineDesc.cullMode = gpu::CullMode::None;
        cubemapRendererPipelineDesc.debugName = "Cubemap renderer pipeline";
        cubemapRendererPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        _cubemapRendererPipeline = device->createGraphicsPipeline(cubemapRendererPipelineDesc);
        _cleanup->push(_cubemapRendererPipeline);
    }

    void resize(gpu::Size2D extent) override
    {
        // draw drains in-flight work before this callback, including first use.
        auto* device = _engine->device();
        auto* surface = _engine->surface();
        release(_cubeReflectionPipeline);
        release(_depth);
        cleanupRings();
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
                                     .debugName = "Mirror cube depth"});
        _cubemapModelUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Sphere model UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(ModelUBO{});
            return buffer;
        });
        _cubemapModelSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_cubemapRendererLayout.get(), 0,
                                                 "Sphere model set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 2}, _cubemapModelUboRing.sharedAt(slot));
            set->update();
            return set;
        });

        _cubeModelUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Cube model UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(ModelUBO{});
            return buffer;
        });
        _cubeModelSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_cubeReflectionLayout.get(), 1,
                                                 "Cube model set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 3}, _cubeModelUboRing.sharedAt(slot));
            set->update();
            return set;
        });

        _cubeSettingsUboRing.create(surface, [device](uint32_t slot)
        {
            auto buffer = device->createBuffer("Cube settings UBO slot[" + std::to_string(slot) + "]");
            buffer->createUniformBuffer(CubeRenderSettingsUBO{});
            return buffer;
        });
        _cubeSettingsSetRing.create(surface, [this, device](uint32_t slot)
        {
            auto set = device->createResourceSet(_cubeReflectionLayout.get(), 3,
                                                 "Cube settings set slot[" + std::to_string(slot) + "]");
            set->setUniformBuffer({.vulkan = 0, .metal = 3}, _cubeSettingsUboRing.sharedAt(slot));
            set->update();
            return set;
        });

        gpu::GraphicsPipelineDescription pipeline{};
        pipeline.vertexShader = _cubeReflectionVs.get();
        pipeline.fragmentShader = _cubeReflectionFs.get();
        pipeline.layout = _cubeReflectionLayout.get();
        pipeline.colorFormat = surface->colorFormat();
        pipeline.depthFormat = _depth->pixelFormat();
        pipeline.debugName = "Mirror cube pipeline";
        pipeline.addVertexBufferDescription(gpu::MeshPNU::vertexBufferDescription());
        _cubeReflectionPipeline = device->createGraphicsPipeline(pipeline);
    }

    void update(const draw::FrameContext& context) override
    {
        _time += context.deltaSeconds;
        _cubemapModelUboRing.at(context.frameSlot)->updateUniformBuffer(ModelUBO{});
        ModelUBO model{};
        model.model = glm::rotate(glm::mat4(1.0f), _time,
                                  glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f)));
        _cubeModelUboRing.at(context.frameSlot)->updateUniformBuffer(model);
        CubeRenderSettingsUBO settings{};
        settings.mode = _controller->debugCubemapMode ? 1u : 0u;
        _cubeSettingsUboRing.at(context.frameSlot)->updateUniformBuffer(settings);
    }

    void render(const draw::FrameContext& context) override
    {
        auto& command = context.commandBuffer;
        // Preserve the original example's six face passes on each active update.
        const auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
        for (uint32_t face = 0; face < 6; ++face)
        {
            const auto& camera = faceCameras[face];
            CubemapCameraPushConstants push{};
            push.projection = projection;
            push.view = glm::lookAt(camera.eye, camera.center, camera.up);
            command.transition(_cubeMap->image(), gpu::ImageLayout::ColorAttachment);
            command.beginRendering(_cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
            command.clearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
            command.bindPipeline(_cubemapRendererPipeline.get());
            command.bindResourceSet(_cubemapRendererPipeline.get(), 0, _cubemapModelSetRing.at(context.frameSlot));
            command.bindResourceSet(_cubemapRendererPipeline.get(), 1, _equirectTextureSet.get());
            command.pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(push), &push);
            _cubemapSphere.draw(&command);
            command.endRendering();
        }
        command.transition(_cubeMap->image(), gpu::ImageLayout::ShaderReadOnly);

        command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
        command.beginRendering(&context.colorTarget, _depth.get());
        const auto& color = _controller->color;
        command.clearColor(0, {color.r, color.g, color.b, color.a});
        command.clearDepth(1.0f);
        CubeReflectionPushConstants push{};
        push.cameraPos = _cameraEye;
        command.bindPipeline(_cubeReflectionPipeline.get());
        command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
        command.bindResourceSet(_cubeReflectionPipeline.get(), 0, _cameraSet.get());
        command.bindResourceSet(_cubeReflectionPipeline.get(), 1, _cubeModelSetRing.at(context.frameSlot));
        command.bindResourceSet(_cubeReflectionPipeline.get(), 2, _cubemapTextureSet.get());
        command.bindResourceSet(_cubeReflectionPipeline.get(), 3, _cubeSettingsSetRing.at(context.frameSlot));
        _cube.draw(&command);
        command.endRendering();
        ++_controller->sceneFrames;
    }

    void cleanup() override
    {
        release(_cubeReflectionPipeline);
        release(_depth);
        cleanupRings();
        _cube.cleanup();
        _cubemapSphere.cleanup();
        if (_cleanup)
        {
            _cleanup->flush();
            _cleanup.reset();
        }
        _cubemapRendererPipeline.reset();
        _cameraSet.reset();
        _cameraUbo.reset();
        _cubemapTextureSet.reset();
        _cubeMap.reset();
        _equirectTextureSet.reset();
        _sampler.reset();
        _equirectTexture.reset();
        _cubeReflectionLayout.reset();
        _cubemapRendererLayout.reset();
        _cubeReflectionFs.reset();
        _cubeReflectionVs.reset();
        _cubemapRendererFs.reset();
        _cubemapRendererVs.reset();
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

    void cleanupRings()
    {
        _cubeSettingsSetRing.cleanup();
        _cubeSettingsSetRing.clear();
        _cubeSettingsUboRing.cleanup();
        _cubeSettingsUboRing.clear();
        _cubeModelSetRing.cleanup();
        _cubeModelSetRing.clear();
        _cubeModelUboRing.cleanup();
        _cubeModelUboRing.clear();
        _cubemapModelSetRing.cleanup();
        _cubemapModelSetRing.clear();
        _cubemapModelUboRing.cleanup();
        _cubemapModelUboRing.clear();
    }

    std::shared_ptr<Controller> _controller;
    float _time = 0.0f;
    const glm::vec3 _cameraEye{0.0f, 0.0f, 3.0f};
    gpu::MeshPU _cubemapSphere;
    gpu::MeshPNU _cube;
    std::unique_ptr<gpu::CleanupManager> _cleanup;
    std::shared_ptr<gpu::Image> _depth;
    std::shared_ptr<gpu::GraphicsPipeline> _cubeReflectionPipeline;
    std::shared_ptr<gpu::ShaderModule> _cubemapRendererVs;
    std::shared_ptr<gpu::ShaderModule> _cubemapRendererFs;
    std::shared_ptr<gpu::ShaderModule> _cubeReflectionVs;
    std::shared_ptr<gpu::ShaderModule> _cubeReflectionFs;
    std::shared_ptr<gpu::PipelineLayout> _cubemapRendererLayout;
    std::shared_ptr<gpu::PipelineLayout> _cubeReflectionLayout;
    std::shared_ptr<gpu::Image> _equirectTexture;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::shared_ptr<gpu::ResourceSet> _equirectTextureSet;
    std::shared_ptr<gpu::CubeMap> _cubeMap;
    std::shared_ptr<gpu::ResourceSet> _cubemapTextureSet;
    std::shared_ptr<gpu::Buffer> _cameraUbo;
    std::shared_ptr<gpu::ResourceSet> _cameraSet;
    std::shared_ptr<gpu::GraphicsPipeline> _cubemapRendererPipeline;
    gpu::FrameResourceRing<gpu::Buffer> _cubemapModelUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _cubemapModelSetRing;
    gpu::FrameResourceRing<gpu::Buffer> _cubeModelUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _cubeModelSetRing;
    gpu::FrameResourceRing<gpu::Buffer> _cubeSettingsUboRing;
    gpu::FrameResourceRing<gpu::ResourceSet> _cubeSettingsSetRing;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller))
    {
    }

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Cubemap controls");
        _window.setPosition(20, 20);
        _window.setSize(380, 340);
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
             ui::Text::text("Cubemap: 512 x 512 per face");
             if (ui::Button::checkBox("Direct cubemap debug", &state.debugCubemapMode))
             {
                 state.loop.requestSceneFrame();
             }
             ui::Text::text("Space toggles mode when scene input is active.");
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
            _controller->debugCubemapMode = !_controller->debugCubemapMode;
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.cubemap");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: cubemap", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_cubemap: " << error.what() << '\n';
        return 1;
    }
}
