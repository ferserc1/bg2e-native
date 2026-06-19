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

// Example 10 — Cubemap rendering with gpu::CubemapRenderPass
//
// This example validates the gpu::CubemapRenderPass utility. Before entering
// the render loop it generates three cubemaps, each one with a single call to
// CubemapRenderPass::render() that takes care of the face/mip iteration, the
// per-face matrices, the begin/end rendering, the viewport/scissor and the
// image transitions:
//
//   1. The original environment cubemap (1024 px) from an equirectangular HDR.
//   2. A diffuse irradiance map (32 px) convolved from the original cubemap.
//   3. A specular reflection map (1024 px) with multiple mip levels down to
//      8x8 px, prefiltered per mip with an increasing roughness.
//
// The render loop only draws a reflective cube using one of those maps:
//   - Space: cycle original -> irradiance -> specular -> original
//   - Enter: when the specular map is active, cycle the sampled mip level
//            (roughness) used to visualize the prefiltered reflection.

#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <vector>

using namespace bg2e;

struct CubemapCameraPushConstants {
    glm::mat4 projection;
    glm::mat4 view;
};

struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 cameraPos;
};

struct ModelUBO {
    glm::mat4 model;
};

struct RoughnessUBO {
    float roughness = 0.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

struct ReflectPushConstants {
    glm::vec3 cameraPos;
    float     lod = 0.0f;
};

int main(int argc, char** argv)
{
    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend to query window type
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. SDL init + create window based on backend window type
    app::initSdlVideoDriver();
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GPU CubemapRenderPass Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags | SDL_WINDOW_RESIZABLE
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 4. Create GPU instance
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create surface
    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    gpu::CleanupManager cleanup(surface.get());

    // 8. Shader modules
    auto shaderBasePath = base::PlatformTools::shaderPath();
    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_cubemap_render_pass");

    auto equirectVs   = shaderLib->vertex("equirect_to_cube", device.get());
    auto equirectFs   = shaderLib->fragment("equirect_to_cube", device.get());
    auto irradianceVs = shaderLib->vertex("irradiance_convolution", device.get());
    auto irradianceFs = shaderLib->fragment("irradiance_convolution", device.get());
    auto specularVs   = shaderLib->vertex("specular_prefilter", device.get());
    auto specularFs   = shaderLib->fragment("specular_prefilter", device.get());
    auto reflectVs    = shaderLib->vertex("reflect_cube", device.get());
    auto reflectFs    = shaderLib->fragment("reflect_cube", device.get());
    cleanup.push(equirectVs);   cleanup.push(equirectFs);
    cleanup.push(irradianceVs); cleanup.push(irradianceFs);
    cleanup.push(specularVs);   cleanup.push(specularFs);
    cleanup.push(reflectVs);    cleanup.push(reflectFs);

    // 9. Pipeline layouts -----------------------------------------------------

    // Equirect -> cube: vertex push constants (proj/view) + set0 (2D texture).
    gpu::PipelineLayoutDescription equirectLayoutDesc{};
    equirectLayoutDesc.pushConstants.push_back(
        {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
    );
    equirectLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1}
    );
    equirectLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1}
    );
    equirectLayoutDesc.debugName = "Equirect to cube pipeline layout";
    auto equirectLayout = device->createPipelineLayout(equirectLayoutDesc);
    cleanup.push(equirectLayout);

    // Irradiance convolution: vertex push constants + set0 (source cubemap).
    gpu::PipelineLayoutDescription irradianceLayoutDesc{};
    irradianceLayoutDesc.pushConstants.push_back(
        {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
    );
    irradianceLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1}
    );
    irradianceLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1}
    );
    irradianceLayoutDesc.debugName = "Irradiance convolution pipeline layout";
    auto irradianceLayout = device->createPipelineLayout(irradianceLayoutDesc);
    cleanup.push(irradianceLayout);

    // Specular prefilter: vertex push constants + set0 (source cubemap) +
    // set1 (per-mip roughness UBO, sampled per mip level).
    gpu::PipelineLayoutDescription specularLayoutDesc{};
    specularLayoutDesc.pushConstants.push_back(
        {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
    );
    specularLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1}
    );
    specularLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1}
    );
    specularLayoutDesc.resourceBindings.push_back(
        {1, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1}
    );
    specularLayoutDesc.debugName = "Specular prefilter pipeline layout";
    auto specularLayout = device->createPipelineLayout(specularLayoutDesc);
    cleanup.push(specularLayout);

    // Reflect cube: fragment push constants (cameraPos + lod), set0 camera UBO,
    // set1 model UBO, set2 cubemap.
    gpu::PipelineLayoutDescription reflectLayoutDesc{};
    reflectLayoutDesc.pushConstants.push_back(
        {0, sizeof(ReflectPushConstants), gpu::ShaderStage::Fragment}
    );
    reflectLayoutDesc.resourceBindings.push_back(
        {0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1}
    );
    reflectLayoutDesc.resourceBindings.push_back(
        {1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1}
    );
    reflectLayoutDesc.resourceBindings.push_back(
        {2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1}
    );
    reflectLayoutDesc.resourceBindings.push_back(
        {2, {.vulkan = 1, .metal = 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1}
    );
    reflectLayoutDesc.debugName = "Reflect cube pipeline layout";
    auto reflectLayout = device->createPipelineLayout(reflectLayoutDesc);
    cleanup.push(reflectLayout);

    // 10. Equirectangular HDR source texture
    auto assetsPath = base::PlatformTools::assetPath();
    auto imageData = std::shared_ptr<base::Image>(bg2e::db::loadImage(assetsPath / "autumn_field_4k.hdr"));
    auto equirectTexture = device->createImage({
        .size = {imageData->width(), imageData->height()},
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
        .debugName = "Equirect HDR texture"
    });
    equirectTexture->uploadImage(imageData.get());
    device->immediateSubmit([equirectTexture](gpu::CommandBuffer* cmd)
    {
        cmd->transition(equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
    });
    cleanup.push(equirectTexture);

    // Linear mip filtering so the specular map can be sampled across its mip
    // chain with textureLod; Repeat addressing keeps the equirect projection
    // seamless and cubemap sampling is seamless regardless of address mode.
    auto sampler = device->createSampler({
        .mipFilter = gpu::MipmapFilter::Linear,
        .debugName = "Default linear sampler"
    });
    cleanup.push(sampler);

    // 11. Cubemap render targets ---------------------------------------------

    // Original environment cubemap, single mip.
    const uint32_t originalSize = 1024;
    auto originalCubeMap = device->createCubeMap({
        .size = originalSize,
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled
               | gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst,
        .mipLevels = 1,
        .debugName = "Original environment cubemap"
    });
    cleanup.push(originalCubeMap);

    // Diffuse irradiance map, small and single mip.
    const uint32_t irradianceSize = 32;
    auto irradianceCubeMap = device->createCubeMap({
        .size = irradianceSize,
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled
               | gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst,
        .mipLevels = 1,
        .debugName = "Irradiance cubemap"
    });
    cleanup.push(irradianceCubeMap);

    // Specular reflection map: same size as the original, with multiple mips.
    // minMipSize = 8 makes the mip chain stop at 8x8 px.
    auto specularCubeMap = device->createCubeMap({
        .size = originalSize,
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled
               | gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst,
        .minMipSize = 8,
        .debugName = "Specular reflection cubemap"
    });
    cleanup.push(specularCubeMap);

    const uint32_t specularMipLevels = specularCubeMap->mipLevels();
    std::cout << "Specular reflection map mip levels: " << specularMipLevels << std::endl;

    // 12. Resource sets used during cubemap generation -----------------------

    auto equirectSet = device->createResourceSet(equirectLayout.get(), 0, "Equirect source set");
    equirectSet->setSampledImage({.vulkan = 0, .metal = 0}, equirectTexture.get());
    equirectSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
    equirectSet->update();
    cleanup.push(equirectSet);

    auto irradianceSourceSet = device->createResourceSet(irradianceLayout.get(), 0, "Irradiance source set");
    irradianceSourceSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, originalCubeMap.get());
    irradianceSourceSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
    irradianceSourceSet->update();
    cleanup.push(irradianceSourceSet);

    auto specularSourceSet = device->createResourceSet(specularLayout.get(), 0, "Specular source set");
    specularSourceSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, originalCubeMap.get());
    specularSourceSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
    specularSourceSet->update();
    cleanup.push(specularSourceSet);

    // One roughness UBO + set per specular mip level. Each mip uses a distinct
    // buffer so they can all be consumed inside a single command buffer.
    std::vector<std::shared_ptr<gpu::Buffer>>      roughnessUbos;
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
        ubo->createUniformBuffer(data);

        auto set = device->createResourceSet(specularLayout.get(), 1,
            "Roughness set mip[" + std::to_string(mip) + "]");
        set->setUniformBuffer({.vulkan = 0, .metal = 1}, ubo);
        set->update();

        roughnessUbos.push_back(ubo);
        roughnessSets.push_back(set);
    }

    // 13. Meshes --------------------------------------------------------------

    // Inverted sphere with equirect UVs to project the HDR into the cubemap.
    std::unique_ptr<bg2e::geo::MeshPU> sphereData(bg2e::geo::createSpherePU(50.0f, 32, 32, true));
    gpu::MeshPU skyboxSphere;
    skyboxSphere.setMeshData(*sphereData);
    skyboxSphere.build(device.get());

    // Unit cube spanning [-1, 1]; its local position is used as the per-face
    // sampling direction for the convolution / prefilter passes.
    std::unique_ptr<bg2e::geo::MeshPU> dirCubeData(bg2e::geo::createCubePU(2.0f, 2.0f, 2.0f));
    gpu::MeshPU dirCube;
    dirCube.setMeshData(*dirCubeData);
    dirCube.build(device.get());

    // On-screen reflective cube.
    std::unique_ptr<bg2e::geo::MeshPNU> reflectCubeData(bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f));
    gpu::MeshPNU reflectCube;
    reflectCube.setMeshData(*reflectCubeData);
    reflectCube.build(device.get());

    // 14. Pipelines -----------------------------------------------------------

    const auto cubeColorFormat = gpu::PixelFormat::R16G16B16A16_SFLOAT;

    gpu::GraphicsPipelineDescription equirectPipelineDesc{};
    equirectPipelineDesc.vertexShader   = equirectVs.get();
    equirectPipelineDesc.fragmentShader = equirectFs.get();
    equirectPipelineDesc.layout         = equirectLayout.get();
    equirectPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    equirectPipelineDesc.colorFormat    = cubeColorFormat;
    equirectPipelineDesc.depthFormat    = gpu::PixelFormat::Undefined;
    equirectPipelineDesc.cullMode       = gpu::CullMode::None;
    equirectPipelineDesc.debugName      = "Equirect to cube pipeline";
    equirectPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto equirectPipeline = device->createGraphicsPipeline(equirectPipelineDesc);
    cleanup.push(equirectPipeline);

    gpu::GraphicsPipelineDescription irradiancePipelineDesc{};
    irradiancePipelineDesc.vertexShader   = irradianceVs.get();
    irradiancePipelineDesc.fragmentShader = irradianceFs.get();
    irradiancePipelineDesc.layout         = irradianceLayout.get();
    irradiancePipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    irradiancePipelineDesc.colorFormat    = cubeColorFormat;
    irradiancePipelineDesc.depthFormat    = gpu::PixelFormat::Undefined;
    irradiancePipelineDesc.cullMode       = gpu::CullMode::None;
    irradiancePipelineDesc.debugName      = "Irradiance convolution pipeline";
    irradiancePipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto irradiancePipeline = device->createGraphicsPipeline(irradiancePipelineDesc);
    cleanup.push(irradiancePipeline);

    gpu::GraphicsPipelineDescription specularPipelineDesc{};
    specularPipelineDesc.vertexShader   = specularVs.get();
    specularPipelineDesc.fragmentShader = specularFs.get();
    specularPipelineDesc.layout         = specularLayout.get();
    specularPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    specularPipelineDesc.colorFormat    = cubeColorFormat;
    specularPipelineDesc.depthFormat    = gpu::PixelFormat::Undefined;
    specularPipelineDesc.cullMode       = gpu::CullMode::None;
    specularPipelineDesc.debugName      = "Specular prefilter pipeline";
    specularPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto specularPipeline = device->createGraphicsPipeline(specularPipelineDesc);
    cleanup.push(specularPipeline);

    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription reflectPipelineDesc{};
    reflectPipelineDesc.vertexShader   = reflectVs.get();
    reflectPipelineDesc.fragmentShader = reflectFs.get();
    reflectPipelineDesc.layout         = reflectLayout.get();
    reflectPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    reflectPipelineDesc.colorFormat    = colorFormat;
    reflectPipelineDesc.depthFormat    = depthFormat;
    reflectPipelineDesc.debugName      = "Reflect cube pipeline";
    reflectPipelineDesc.addVertexBufferDescription(gpu::MeshPNU::vertexBufferDescription());
    auto reflectPipeline = device->createGraphicsPipeline(reflectPipelineDesc);
    cleanup.push(reflectPipeline);

    // 15. Generate the three cubemaps once, before the render loop -----------
    //
    // Each generation step is driven by gpu::CubemapRenderPass, which iterates
    // the six faces (and, for the specular map, every mip level), sets up the
    // matrices, viewport/scissor, begin/end rendering and image transitions.
    gpu::CubemapRenderPassInfo genInfo{};
    genInfo.clear           = true;
    genInfo.clearColor      = gpu::Color(0.0f, 0.0f, 0.0f, 1.0f);
    genInfo.transitionBefore = true;
    genInfo.transitionAfter  = true;

    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        // 1) Original environment cubemap from the equirectangular texture.
        gpu::CubemapRenderPass::render(cmd, originalCubeMap.get(), genInfo,
            [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t,
                const glm::mat4& projection, const glm::mat4& view)
            {
                CubemapCameraPushConstants pc{ projection, view };
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
                CubemapCameraPushConstants pc{ projection, view };
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
                CubemapCameraPushConstants pc{ projection, view };
                c->bindPipeline(specularPipeline.get());
                c->bindResourceSet(specularPipeline.get(), 0, specularSourceSet.get());
                c->bindResourceSet(specularPipeline.get(), 1, roughnessSets[mipLevel].get());
                c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
                dirCube.draw(c);
            });
    });

    // 16. On-screen resources -------------------------------------------------

    const float aspect = 800.0f / 600.0f;
    glm::vec3 cameraEye(0.0f, 0.0f, 3.0f);
    CameraUBO cameraData{};
    cameraData.projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
    cameraData.view = glm::lookAt(cameraEye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cameraData.cameraPos = glm::vec4(cameraEye, 1.0f);

    auto cameraUbo = device->createBuffer("Camera UBO");
    cameraUbo->createUniformBuffer(cameraData);
    cleanup.push(cameraUbo);

    auto cameraSet = device->createResourceSet(reflectLayout.get(), 0, "Camera resource set");
    cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
    cameraSet->update();
    cleanup.push(cameraSet);

    // One cubemap resource set per generated map (set 2), selected with Space.
    auto makeCubeSet = [&](gpu::CubeMap* map, const std::string& name)
    {
        auto set = device->createResourceSet(reflectLayout.get(), 2, name);
        set->setSampledCubeMap({.vulkan = 0, .metal = 0}, map);
        set->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
        set->update();
        cleanup.push(set);
        return set;
    };
    std::array<std::shared_ptr<gpu::ResourceSet>, 3> cubeSets = {
        makeCubeSet(originalCubeMap.get(),  "Reflect original cube set"),
        makeCubeSet(irradianceCubeMap.get(), "Reflect irradiance cube set"),
        makeCubeSet(specularCubeMap.get(),  "Reflect specular cube set")
    };
    const char* mapNames[3] = { "original cubemap", "irradiance map", "specular reflection map" };

    // Per-frame model UBO for the reflective cube.
    gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
    modelUboRing.create(surface.get(), [&](uint32_t i)
    {
        auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
        buffer->createUniformBuffer(ModelUBO{});
        return buffer;
    });

    gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
    modelSetRing.create(surface.get(), [&](uint32_t i)
    {
        auto set = device->createResourceSet(reflectLayout.get(), 1, "Model set ring[" + std::to_string(i) + "]");
        set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 17. Render loop ---------------------------------------------------------
    auto& graphicsQueue = device->graphicsQueue();

    int activeMap   = 0;   // 0 = original, 1 = irradiance, 2 = specular
    uint32_t specularLod = 0;

    std::cout << "Active reflection map: " << mapNames[activeMap] << std::endl;
    std::cout << "Controls: [Space] cycle map, [Enter] cycle specular mip level" << std::endl;

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                device->waitIdle();
                surface->resize({
                    static_cast<uint32_t>(event.window.data1),
                    static_cast<uint32_t>(event.window.data2)
                });
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
            {
                activeMap = (activeMap + 1) % 3;
                std::cout << "Active reflection map: " << mapNames[activeMap] << std::endl;
            }
            if (event.type == SDL_KEYDOWN &&
                (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER))
            {
                specularLod = (specularLod + 1) % specularMipLevels;
                std::cout << "Specular reflection mip level: " << specularLod
                          << " / " << (specularMipLevels - 1) << std::endl;
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");
        cmd->begin();

        // Update the rotating cube model matrix.
        ModelUBO modelData{};
        modelData.model = glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f)));
        auto* modelUbo = modelUboRing.current();
        modelUbo->updateUniformBuffer(modelData);
        auto* modelSet = modelSetRing.current();

        // Only the specular map exposes meaningful mip levels.
        ReflectPushConstants reflectPush{};
        reflectPush.cameraPos = cameraEye;
        reflectPush.lod = (activeMap == 2) ? float(specularLod) : 0.0f;

        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
        cmd->clearDepth(1.0f);

        cmd->bindPipeline(reflectPipeline.get());
        cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(reflectPush), &reflectPush);
        cmd->bindResourceSet(reflectPipeline.get(), 0, cameraSet.get());
        cmd->bindResourceSet(reflectPipeline.get(), 1, modelSet);
        cmd->bindResourceSet(reflectPipeline.get(), 2, cubeSets[activeMap].get());
        reflectCube.draw(cmd.get());

        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 18. Cleanup
    device->waitIdle();

    modelSetRing.cleanup();
    modelUboRing.cleanup();
    skyboxSphere.cleanup();
    dirCube.cleanup();
    reflectCube.cleanup();

    cleanup.flush();

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
