#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/CubemapRenderPass.hpp>
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

struct CubemapCameraPushConstants {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
};

struct CameraUBO {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 cameraPos;
};

struct ModelUBO {
    glm::mat4 model{1.0f};
};

struct RoughnessUBO {
    float roughness = 0.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

struct ReflectPushConstants {
    glm::vec3 cameraPos;
    float lod = 0.0f;
};

static_assert(sizeof(ReflectPushConstants) == 16);
static_assert(offsetof(ReflectPushConstants, lod) == 12);
static_assert(sizeof(RoughnessUBO) == 16);

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
        // Shader modules
        auto shaderBasePath = base::PlatformTools::shaderPath();
        gpu::ShaderLib shaderLib(shaderBasePath / "draw_cubemap_render_pass", _engine->backendType());

        auto equirectVs = shaderLib.vertex("equirect_to_cube", device);
        auto equirectFs = shaderLib.fragment("equirect_to_cube", device);
        auto irradianceVs = shaderLib.vertex("irradiance_convolution", device);
        auto irradianceFs = shaderLib.fragment("irradiance_convolution", device);
        auto specularVs = shaderLib.vertex("specular_prefilter", device);
        auto specularFs = shaderLib.fragment("specular_prefilter", device);
        auto reflectVs = shaderLib.vertex("reflect_cube", device);
        auto reflectFs = shaderLib.fragment("reflect_cube", device);
        _cleanup->push(equirectVs);
        _cleanup->push(equirectFs);
        _cleanup->push(irradianceVs);
        _cleanup->push(irradianceFs);
        _cleanup->push(specularVs);
        _cleanup->push(specularFs);
        _cleanup->push(reflectVs);
        _cleanup->push(reflectFs);

        // Pipeline layouts -----------------------------------------------------

        // Equirect -> cube: vertex push constants (proj/view) + set0 (2D texture).
        gpu::PipelineLayoutDescription equirectLayoutDesc{};
        equirectLayoutDesc.pushConstants.push_back(
            {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex});
        equirectLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        equirectLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        equirectLayoutDesc.debugName = "Equirect to cube pipeline layout";
        auto equirectLayout = device->createPipelineLayout(equirectLayoutDesc);
        _cleanup->push(equirectLayout);

        // Irradiance convolution: vertex push constants + set0 (source cubemap).
        gpu::PipelineLayoutDescription irradianceLayoutDesc{};
        irradianceLayoutDesc.pushConstants.push_back(
            {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex});
        irradianceLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        irradianceLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        irradianceLayoutDesc.debugName = "Irradiance convolution pipeline layout";
        auto irradianceLayout = device->createPipelineLayout(irradianceLayoutDesc);
        _cleanup->push(irradianceLayout);

        // Specular prefilter: vertex push constants + set0 (source cubemap) +
        // set1 (per-mip roughness UBO, sampled per mip level).
        gpu::PipelineLayoutDescription specularLayoutDesc{};
        specularLayoutDesc.pushConstants.push_back(
            {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex});
        specularLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        specularLayoutDesc.resourceBindings.push_back(
            {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        specularLayoutDesc.resourceBindings.push_back(
            {1, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
        specularLayoutDesc.debugName = "Specular prefilter pipeline layout";
        auto specularLayout = device->createPipelineLayout(specularLayoutDesc);
        _cleanup->push(specularLayout);

        // Reflect cube: fragment push constants (cameraPos + lod), set0 camera UBO,
        // set1 model UBO, set2 cubemap.
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

        // Equirectangular HDR source texture
        auto assetsPath = base::PlatformTools::assetPath();
        auto imageData = std::shared_ptr<base::Image>(db::loadImage(assetsPath / "autumn_field_4k.hdr"));
        if (!imageData || imageData->width() == 0 || imageData->height() == 0)
        {
            throw std::runtime_error("Failed to load the cubemap HDR environment.");
        }
        auto equirectTexture = device->createImage({.size = {imageData->width(), imageData->height()},
                                                    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                                    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                                    .debugName = "Equirect HDR texture"});
        _cleanup->push(equirectTexture);
        equirectTexture->uploadImage(imageData.get());
        device->immediateSubmit([equirectTexture](gpu::CommandBuffer* cmd)
                                {
                                    cmd->transition(equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
                                });

        // Linear mip filtering so the specular map can be sampled across its mip
        // chain with textureLod; Repeat addressing keeps the equirect projection
        // seamless and cubemap sampling is seamless regardless of address mode.
        auto sampler = device->createSampler({.mipFilter = gpu::MipmapFilter::Linear,
                                              .debugName = "Default linear sampler"});
        _cleanup->push(sampler);

        // Cubemap render targets ---------------------------------------------

        // Original environment cubemap, single mip.
        const uint32_t originalSize = 1024;
        auto originalCubeMap = device->createCubeMap({.size = originalSize,
                                                      .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                                      .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
                                                      .mipLevels = 1,
                                                      .debugName = "Original environment cubemap"});
        _cleanup->push(originalCubeMap);

        // Diffuse irradiance map, small and single mip.
        const uint32_t irradianceSize = 32;
        auto irradianceCubeMap = device->createCubeMap({.size = irradianceSize,
                                                        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                                        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
                                                        .mipLevels = 1,
                                                        .debugName = "Irradiance cubemap"});
        _cleanup->push(irradianceCubeMap);

        // Specular reflection map: same size as the original, with multiple mips.
        // minMipSize = 8 makes the mip chain stop at 8x8 px.
        auto specularCubeMap = device->createCubeMap({.size = originalSize,
                                                      .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                                      .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
                                                      .minMipSize = 8,
                                                      .debugName = "Specular reflection cubemap"});
        _cleanup->push(specularCubeMap);

        const uint32_t specularMipLevels = specularCubeMap->mipLevels();
        _controller->specularMipLevels = specularMipLevels;

        // Resource sets used during cubemap generation -----------------------

        auto equirectSet = device->createResourceSet(equirectLayout.get(), 0, "Equirect source set");
        equirectSet->setSampledImage({.vulkan = 0, .metal = 0}, equirectTexture.get());
        equirectSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
        equirectSet->update();
        _cleanup->push(equirectSet);

        auto irradianceSourceSet = device->createResourceSet(irradianceLayout.get(), 0, "Irradiance source set");
        irradianceSourceSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, originalCubeMap.get());
        irradianceSourceSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
        irradianceSourceSet->update();
        _cleanup->push(irradianceSourceSet);

        auto specularSourceSet = device->createResourceSet(specularLayout.get(), 0, "Specular source set");
        specularSourceSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, originalCubeMap.get());
        specularSourceSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
        specularSourceSet->update();
        _cleanup->push(specularSourceSet);

        // One roughness UBO + set per specular mip level. Each mip uses a distinct
        // buffer so they can all be consumed inside a single command buffer.
        std::vector<std::shared_ptr<gpu::Buffer>> roughnessUbos;
        std::vector<std::shared_ptr<gpu::ResourceSet>> roughnessSets;
        roughnessUbos.reserve(specularMipLevels);
        roughnessSets.reserve(specularMipLevels);
        for (uint32_t mip = 0; mip < specularMipLevels; ++mip)
        {
            RoughnessUBO data{};
            data.roughness = specularMipLevels > 1
                                 ? float(mip) / float(specularMipLevels - 1)
                                 : 0.0f;

            auto ubo = device->createBuffer("Roughness UBO mip[" + std::to_string(mip) + "]");
            _cleanup->push(ubo);
            ubo->createUniformBuffer(data);

            auto set = device->createResourceSet(specularLayout.get(), 1,
                                                 "Roughness set mip[" + std::to_string(mip) + "]");
            _cleanup->push(set);
            set->setUniformBuffer({.vulkan = 0, .metal = 1}, ubo);
            set->update();

            roughnessUbos.push_back(ubo);
            roughnessSets.push_back(set);
        }

        // Meshes --------------------------------------------------------------

        // Inverted sphere with equirect UVs to project the HDR into the cubemap.
        std::unique_ptr<bg2e::geo::MeshPU> sphereData(bg2e::geo::createSpherePU(50.0f, 32, 32, true));
        gpu::MeshPU skyboxSphere;
        skyboxSphere.setMeshData(*sphereData);
        skyboxSphere.build(device);

        // Unit cube spanning [-1, 1]; its local position is used as the per-face
        // sampling direction for the convolution / prefilter passes.
        std::unique_ptr<bg2e::geo::MeshPU> dirCubeData(bg2e::geo::createCubePU(2.0f, 2.0f, 2.0f));
        gpu::MeshPU dirCube;
        dirCube.setMeshData(*dirCubeData);
        dirCube.build(device);

        // On-screen reflective cube.
        std::unique_ptr<bg2e::geo::MeshPNU> reflectCubeData(bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f));
        _cube.setMeshData(*reflectCubeData);
        _cube.build(device);

        // Pipelines -----------------------------------------------------------

        const auto cubeColorFormat = gpu::PixelFormat::R16G16B16A16_SFLOAT;

        gpu::GraphicsPipelineDescription equirectPipelineDesc{};
        equirectPipelineDesc.vertexShader = equirectVs.get();
        equirectPipelineDesc.fragmentShader = equirectFs.get();
        equirectPipelineDesc.layout = equirectLayout.get();
        equirectPipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
        equirectPipelineDesc.colorFormat = cubeColorFormat;
        equirectPipelineDesc.depthFormat = gpu::PixelFormat::Undefined;
        equirectPipelineDesc.cullMode = gpu::CullMode::None;
        equirectPipelineDesc.debugName = "Equirect to cube pipeline";
        equirectPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        auto equirectPipeline = device->createGraphicsPipeline(equirectPipelineDesc);
        _cleanup->push(equirectPipeline);

        gpu::GraphicsPipelineDescription irradiancePipelineDesc{};
        irradiancePipelineDesc.vertexShader = irradianceVs.get();
        irradiancePipelineDesc.fragmentShader = irradianceFs.get();
        irradiancePipelineDesc.layout = irradianceLayout.get();
        irradiancePipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
        irradiancePipelineDesc.colorFormat = cubeColorFormat;
        irradiancePipelineDesc.depthFormat = gpu::PixelFormat::Undefined;
        irradiancePipelineDesc.cullMode = gpu::CullMode::None;
        irradiancePipelineDesc.debugName = "Irradiance convolution pipeline";
        irradiancePipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        auto irradiancePipeline = device->createGraphicsPipeline(irradiancePipelineDesc);
        _cleanup->push(irradiancePipeline);

        gpu::GraphicsPipelineDescription specularPipelineDesc{};
        specularPipelineDesc.vertexShader = specularVs.get();
        specularPipelineDesc.fragmentShader = specularFs.get();
        specularPipelineDesc.layout = specularLayout.get();
        specularPipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
        specularPipelineDesc.colorFormat = cubeColorFormat;
        specularPipelineDesc.depthFormat = gpu::PixelFormat::Undefined;
        specularPipelineDesc.cullMode = gpu::CullMode::None;
        specularPipelineDesc.debugName = "Specular prefilter pipeline";
        specularPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
        auto specularPipeline = device->createGraphicsPipeline(specularPipelineDesc);
        _cleanup->push(specularPipeline);

        // Generate the three cubemaps once, before the render loop -----------
        //
        // Each generation step is driven by gpu::CubemapRenderPass, which iterates
        // the six faces (and, for the specular map, every mip level), sets up the
        // matrices, viewport/scissor, begin/end rendering and image transitions.
        gpu::CubemapRenderPassInfo genInfo{};
        genInfo.clear = true;
        genInfo.clearColor = gpu::Color(0.0f, 0.0f, 0.0f, 1.0f);
        genInfo.transitionBefore = true;
        genInfo.transitionAfter = true;

        device->immediateSubmit([&](gpu::CommandBuffer* cmd)
                                {
                                    // 1) Original environment cubemap from the equirectangular texture.
                                    gpu::CubemapRenderPass::render(cmd, originalCubeMap.get(), genInfo,
                                                                   [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t,
                                                                       const glm::mat4& projection, const glm::mat4& view)
                                                                   {
                                                                       CubemapCameraPushConstants pc{projection, view};
                                                                       c->bindPipeline(equirectPipeline.get());
                                                                       c->bindResourceSet(equirectPipeline.get(), 0, equirectSet.get());
                                                                       c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
                                                                       skyboxSphere.draw(c);
                                                                   });

                                    // 2) Diffuse irradiance map convolved from the original cubemap.
                                    gpu::CubemapRenderPass::render(cmd, irradianceCubeMap.get(), genInfo,
                                                                   [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t,
                                                                       const glm::mat4& projection, const glm::mat4& view)
                                                                   {
                                                                       CubemapCameraPushConstants pc{projection, view};
                                                                       c->bindPipeline(irradiancePipeline.get());
                                                                       c->bindResourceSet(irradiancePipeline.get(), 0, irradianceSourceSet.get());
                                                                       c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
                                                                       dirCube.draw(c);
                                                                   });

                                    // 3) Specular reflection map: each mip level is prefiltered with an
                                    //    increasing roughness (roughness = mip / (mipCount - 1)).
                                    gpu::CubemapRenderPass::render(cmd, specularCubeMap.get(), genInfo,
                                                                   [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t mipLevel,
                                                                       const glm::mat4& projection, const glm::mat4& view)
                                                                   {
                                                                       CubemapCameraPushConstants pc{projection, view};
                                                                       c->bindPipeline(specularPipeline.get());
                                                                       c->bindResourceSet(specularPipeline.get(), 0, specularSourceSet.get());
                                                                       c->bindResourceSet(specularPipeline.get(), 1, roughnessSets[mipLevel].get());
                                                                       c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
                                                                       dirCube.draw(c);
                                                                   });
                                });

        // immediateSubmit has completed: conversion-only mesh buffers can retire.
        skyboxSphere.cleanup();
        dirCube.cleanup();
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

        auto makeCubeSet = [this, device, &sampler](gpu::CubeMap* map, const std::string& name)
        {
            auto set = device->createResourceSet(_reflectLayout.get(), 2, name);
            _cleanup->push(set);
            set->setSampledCubeMap({.vulkan = 0, .metal = 0}, map);
            set->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
            set->update();
            return set;
        };
        _cubeSets = {
            makeCubeSet(originalCubeMap.get(), "Reflect original cube set"),
            makeCubeSet(irradianceCubeMap.get(), "Reflect irradiance cube set"),
            makeCubeSet(specularCubeMap.get(), "Reflect specular cube set")
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
