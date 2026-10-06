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

#include <SDL2/SDL.h>

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/IntegratedBakerContext.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/uniforms/materials.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Mesh.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/vk/FrameDataBinding.hpp>
#include <bg2e/scene/vk/ObjectDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <typename Fn>
void requireThrows(Fn&& fn, const std::string& message)
{
    bool threw = false;
    try
    {
        fn();
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    require(threw, message);
}

void testBakedMaterialFlag()
{
    bg2e::base::MaterialAttributes attributes;
    bg2e::render::uniforms::PBRMaterialData material{};
    material = attributes;
    require((material.flags & bg2e::render::uniforms::PBRMaterialData::HAS_BAKED_LIGHTMAP) == 0,
        "material without an explicit AO texture was marked as baked");

    attributes.setAoTexture(std::make_shared<bg2e::base::Texture>(std::string("fixture_lightmap.png")));
    material = attributes;
    require((material.flags & bg2e::render::uniforms::PBRMaterialData::HAS_BAKED_LIGHTMAP) != 0,
        "explicit AO texture did not set the internal baked-lightmap flag");
}

class SolidRgbLightmapGenerator final : public bg2e::base::ProceduralTextureGenerator
{
public:
    SolidRgbLightmapGenerator()
        : ProceduralTextureGenerator(2, 2, 4)
    {}

    uint8_t* generate() override
    {
        auto* pixels = new uint8_t[width() * height() * 4];
        for (uint32_t pixel = 0; pixel < width() * height(); ++pixel)
        {
            pixels[pixel * 4 + 0] = 230;
            pixels[pixel * 4 + 1] = 82;
            pixels[pixel * 4 + 2] = 31;
            pixels[pixel * 4 + 3] = 255;
        }
        return pixels;
    }

    std::string imageIdentifier() override { return "lightmap-fixture-rgb-230-82-31"; }
    bg2e::base::Color::Type colorType() override { return bg2e::base::Color::TypeLinear; }
};

void verifyBakedRgbGBuffer(
    bg2e::render::Engine& engine,
    bg2e::render::vulkan::FrameResources& frameResources,
    bg2e::scene::vk::FrameDataBinding& frameDataBinding,
    bg2e::scene::vk::ObjectDataBinding& objectDataBinding,
    bg2e::scene::DrawableComponent& drawableComponent)
{
    using namespace bg2e;
    using namespace bg2e::render::vulkan;
    render::GBufferManager gbuffer(&engine);
    gbuffer.build({ 64, 64 });

    factory::PipelineLayout layoutFactory(&engine);
    layoutFactory.addDescriptorSetLayout(frameDataBinding.createLayout());
    layoutFactory.addDescriptorSetLayout(objectDataBinding.createLayout());
    const VkPipelineLayout pipelineLayout = layoutFactory.build("BakedRgbFixture::PipelineLayout");

    factory::GraphicsPipeline pipelineFactory(&engine);
    pipelineFactory.setInputState<scene::Drawable>();
    pipelineFactory.disableMultisample();
    pipelineFactory.setDepthFormat(gbuffer.depthFormat());
    pipelineFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    pipelineFactory.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineFactory.setColorAttachmentFormat(gbuffer.formats());
    pipelineFactory.addShader("deferred_gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("deferred_gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    const VkPipeline pipeline = pipelineFactory.build(pipelineLayout, "BakedRgbFixture::Pipeline");

    engine.command().immediateSubmit([&](VkCommandBuffer cmd) {
        gbuffer.beginRender(cmd, false);
        macros::cmdSetDefaultViewportAndScissor(cmd, gbuffer.extent());
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        const VkDescriptorSet sceneSet = frameDataBinding.newDescriptorSet(
            frameResources, glm::mat4(1.0f), glm::mat4(1.0f));
        drawableComponent.draw(glm::mat4(1.0f), cmd, pipelineLayout,
            [&](render::MaterialBase* material, const glm::mat4& transform, uint32_t) {
                const VkDescriptorSet objectSet = objectDataBinding.newDescriptorSet(
                    frameResources, material, transform);
                return std::vector<VkDescriptorSet>{ sceneSet, objectSet };
            });
        cmdEndRendering(cmd);
        gbuffer.transitionToShaderRead(cmd);
    });

    std::vector<uint8_t> bakedPixels;
    std::vector<uint8_t> flagsPixels;
    Image::readPixelsRGBA8(&engine, gbuffer.image(5).get(), 0, 0, 64, 64,
        bakedPixels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Image::readPixelsRGBA8(&engine, gbuffer.image(3).get(), 0, 0, 64, 64,
        flagsPixels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    bool foundRgbSample = false;
    bool foundBakedFlag = false;
    for (size_t pixel = 0; pixel < 64u * 64u; ++pixel)
    {
        const size_t offset = pixel * 4;
        if (bakedPixels[offset + 3] == 0)
        {
            continue;
        }
        const int red = bakedPixels[offset + 0];
        const int green = bakedPixels[offset + 1];
        const int blue = bakedPixels[offset + 2];
        foundRgbSample = foundRgbSample ||
            (std::abs(red - 230) < 4 && std::abs(green - 82) < 4 && std::abs(blue - 31) < 4);
        foundBakedFlag = foundBakedFlag || (flagsPixels[offset + 3] & (1u << 3)) != 0u;
    }
    require(foundRgbSample, "G-buffer baked-lightmap attachment collapsed or lost RGB channels");
    require(foundBakedFlag, "G-buffer omitted the internal baked-lightmap material flag");

    engine.device().waitIdle();
    frameResources.cleanupManager.flush(engine.device());
    frameResources.descriptorAllocator->clearDescriptors();
    vkDestroyPipeline(engine.device().handle(), pipeline, nullptr);
    vkDestroyPipelineLayout(engine.device().handle(), pipelineLayout, nullptr);
    frameDataBinding.cleanup();
    objectDataBinding.cleanup();
}

std::shared_ptr<bg2e::scene::Mesh> makeTriangleMesh()
{
    using namespace bg2e;
    auto mesh = std::make_shared<scene::Mesh>();

    const auto vertex = [](glm::vec3 position, glm::vec2 uv) {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv;
        result.texCoord1 = uv;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };

    mesh->vertices = {
        vertex({-0.8f, -0.8f, 0.0f}, {0.1f, 0.1f}),
        vertex({ 0.8f, -0.8f, 0.0f}, {0.9f, 0.1f}),
        vertex({-0.8f,  0.8f, 0.0f}, {0.1f, 0.9f})
    };
    mesh->indices = { 0, 1, 2 };
    mesh->submeshes = { { 0, 3 } };
    return mesh;
}

std::shared_ptr<bg2e::scene::Mesh> makeOverhangMesh()
{
    using namespace bg2e;
    auto mesh = std::make_shared<scene::Mesh>();
    const auto vertex = [](glm::vec3 position, glm::vec2 uv) {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv;
        result.texCoord1 = uv;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };
    mesh->vertices = {
        vertex({-0.55f, -0.55f, 0.08f}, {0.0f, 0.0f}),
        vertex({-0.10f, -0.55f, 0.08f}, {1.0f, 0.0f}),
        vertex({-0.10f, -0.10f, 0.08f}, {1.0f, 1.0f}),
        vertex({-0.55f, -0.10f, 0.08f}, {0.0f, 1.0f})
    };
    mesh->indices = { 0, 1, 2, 0, 2, 3 };
    mesh->submeshes = { { 0, 6 } };
    return mesh;
}

float halfToFloat(uint16_t half)
{
    const uint32_t sign = (half >> 15) & 1u;
    const uint32_t exponent = (half >> 10) & 0x1fu;
    const uint32_t mantissa = half & 0x3ffu;
    float value;
    if (exponent == 0u)
    {
        value = std::ldexp(static_cast<float>(mantissa), -24);
    }
    else if (exponent == 0x1fu)
    {
        value = mantissa == 0u ? std::numeric_limits<float>::infinity()
                               : std::numeric_limits<float>::quiet_NaN();
    }
    else
    {
        value = std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f,
                           static_cast<int>(exponent) - 15);
    }
    return sign ? -value : value;
}

std::vector<glm::vec4> readLightmapImage(
    bg2e::render::Engine& engine,
    const bg2e::render::vulkan::Image& image)
{
    const VkExtent2D extent = image.extent2D();
    const VkDeviceSize pixelCount = static_cast<VkDeviceSize>(extent.width) * extent.height;
    const VkDeviceSize byteCount = pixelCount * sizeof(uint16_t) * 4;
    std::unique_ptr<bg2e::render::vulkan::Buffer> staging(
        bg2e::render::vulkan::Buffer::createAllocatedBuffer(
            &engine,
            byteCount,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_TO_CPU,
            "Lightmap fixture readback"));

    engine.command().immediateSubmit([&](VkCommandBuffer cmd) {
        bg2e::render::vulkan::Image::TransitionInfo toTransfer(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT);
        bg2e::render::vulkan::Image::cmdTransitionImage(
            cmd, image.handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, toTransfer);

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = { extent.width, extent.height, 1 };
        vkCmdCopyImageToBuffer(cmd, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            staging->handle(), 1, &copy);

        bg2e::render::vulkan::Image::TransitionInfo toSampling(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        bg2e::render::vulkan::Image::cmdTransitionImage(
            cmd, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, toSampling);
    });

    VK_ASSERT(vmaInvalidateAllocation(engine.allocator(), staging->allocation(), 0, VK_WHOLE_SIZE));
    const auto* halfPixels = static_cast<const uint16_t*>(staging->allocatedData());
    std::vector<glm::vec4> pixels(static_cast<size_t>(pixelCount));
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = glm::vec4(
            halfToFloat(halfPixels[i * 4 + 0]),
            halfToFloat(halfPixels[i * 4 + 1]),
            halfToFloat(halfPixels[i * 4 + 2]),
            halfToFloat(halfPixels[i * 4 + 3]));
    }
    return pixels;
}

glm::vec4 sampleAtUv(const std::vector<glm::vec4>& pixels, uint32_t resolution, float u, float v)
{
    const uint32_t x = std::min(static_cast<uint32_t>(u * resolution), resolution - 1);
    const uint32_t y = std::min(static_cast<uint32_t>((1.0f - v) * resolution), resolution - 1);
    return pixels[static_cast<size_t>(y) * resolution + x];
}

class RenderDelegateFixture
{
public:
    RenderDelegateFixture(
        bg2e::render::Engine& engine,
        bg2e::render::IntegratedBakerContext& context,
        bg2e::render::IntegratedLightmapBaker& baker,
        bg2e::render::EnvironmentResources& environmentResources,
        const std::shared_ptr<bg2e::scene::Node>& root,
        const std::shared_ptr<bg2e::scene::Node>& target)
        : _engine(engine), _context(context), _environmentResources(environmentResources),
          _root(root), _target(target)
    {
        _bakers.push_back(&baker);
    }

    void addBaker(bg2e::render::IntegratedLightmapBaker& baker)
    {
        _bakers.push_back(&baker);
    }

    void recordFrame(bool ordinaryRendererFirst, bool testMovedTarget = false)
    {
        auto& frame = _engine.currentFrameResources();
        require(frame.commandBuffer != VK_NULL_HANDLE, "current frame has no command buffer");
        require(frame.frameFence != VK_NULL_HANDLE, "current frame has no fence");
        require(frame.descriptorAllocator != nullptr, "current frame has no descriptor allocator");

        VK_ASSERT(vkWaitForFences(_engine.device().handle(), 1, &frame.frameFence, VK_TRUE, UINT64_MAX));
        VK_ASSERT(vkResetFences(_engine.device().handle(), 1, &frame.frameFence));
        VK_ASSERT(vkResetCommandBuffer(frame.commandBuffer, 0));
        if (frame.rayTracingScene)
        {
            frame.rayTracingScene->cleanup();
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_ASSERT(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo));
        _environmentResources.update(frame.commandBuffer, _engine.currentFrame(), frame);

        auto* ordinaryScene = frame.rayTracingScene;
        struct RestoreScenePointer
        {
            bg2e::render::vulkan::FrameResources& frame;
            bg2e::render::vulkan::rt::RayTracingScene* scene;
            bool active = false;
            ~RestoreScenePointer()
            {
                if (active)
                {
                    frame.rayTracingScene = scene;
                }
            }
        } restore{ frame, ordinaryScene };

        if (ordinaryRendererFirst)
        {
            require(ordinaryScene->update(frame.commandBuffer, _root.get()),
                "ordinary frame TLAS update failed");
            require(ordinaryScene->objectInstances().size() >= 2,
                "overhang fixture geometry was not included in the scene TLAS");
        }
        else
        {
            // The ordinary renderer's per-frame TLAS is hidden while the bake
            // context builds and consumes its independent TLAS.
            frame.rayTracingScene = nullptr;
            restore.active = true;
            requireThrows([&]() { _bakers.front()->update(frame.commandBuffer, frame); },
                "baker update before prepareFrame was accepted");
        }

        _context.prepareFrame(frame.commandBuffer, frame);

        if (testMovedTarget)
        {
            auto otherRoot = std::make_shared<bg2e::scene::Node>("other root");
            _root->removeChild(_target);
            otherRoot->addChild(_target);
            requireThrows([&]() { _bakers.front()->update(frame.commandBuffer, frame); },
                "baker accepted a target moved to another root");
            otherRoot->removeChild(_target);
            _root->addChild(_target);
        }

        for (auto* baker : _bakers)
        {
            baker->update(frame.commandBuffer, frame);
        }
        for (auto* baker : _bakers)
        {
            requireThrows([&]() { baker->update(frame.commandBuffer, frame); },
                "baker accepted two updates in one engine frame");
        }

        if (!ordinaryRendererFirst)
        {
            frame.rayTracingScene = ordinaryScene;
            restore.active = false;
            require(ordinaryScene->update(frame.commandBuffer, _root.get()),
                "ordinary frame TLAS update after baking failed");
        }

        VK_ASSERT(vkEndCommandBuffer(frame.commandBuffer));

        VkCommandBufferSubmitInfo commandInfo{};
        commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandInfo.commandBuffer = frame.commandBuffer;
        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandInfo;
        VK_ASSERT(bg2e::render::vulkan::queueSubmit2(
            _engine.device().graphicsQueue(), 1, &submitInfo, frame.frameFence));
    }

private:
    bg2e::render::Engine& _engine;
    bg2e::render::IntegratedBakerContext& _context;
    bg2e::render::EnvironmentResources& _environmentResources;
    std::vector<bg2e::render::IntegratedLightmapBaker*> _bakers;
    std::shared_ptr<bg2e::scene::Node> _root;
    std::shared_ptr<bg2e::scene::Node> _target;
};

}

int main()
{
    try
    {
        testBakedMaterialFlag();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Baked material flag fixture failed: " << error.what() << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "Integrated baker scene-binding fixture skipped: SDL video is unavailable" << std::endl;
        return 0;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Integrated baker scene-binding fixture",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        64,
        64,
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN
    );
    if (!window)
    {
        std::cout << "Integrated baker scene-binding fixture skipped: no Vulkan window is available" << std::endl;
        SDL_Quit();
        return 0;
    }

    bg2e::render::Engine engine;
    bool engineInitialized = false;
    std::unique_ptr<bg2e::render::EnvironmentResources> environmentResources;
    try
    {
        engine.init(window);
        engineInitialized = true;
        engine.iterateFrameResources([&](bg2e::render::vulkan::FrameResources& frame) {
            frame.descriptorAllocator->init(&engine);
        });

        if (!engine.rayTracingSupported())
        {
            std::cout << "Integrated baker scene-binding fixture skipped: ray tracing is unsupported" << std::endl;
            engine.cleanup();
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 0;
        }

        auto environmentTexture = std::shared_ptr<bg2e::render::Texture>(
            bg2e::render::Texture::colorTexture(
                &engine, bg2e::base::Color(0.45f, 0.65f, 0.8f, 1.0f), { 8, 4 }));
        environmentResources = std::make_unique<bg2e::render::EnvironmentResources>(&engine);
        bg2e::scene::vk::FrameDataBinding frameDataBinding(&engine);
        bg2e::scene::vk::ObjectDataBinding objectDataBinding(&engine);
        engine.iterateFrameResources([&](bg2e::render::vulkan::FrameResources& frame) {
            environmentResources->initFrameResources(frame.descriptorAllocator);
            frameDataBinding.initFrameResources(frame.descriptorAllocator);
            objectDataBinding.initFrameResources(frame.descriptorAllocator);
        });
        engine.iterateFrameResources([](bg2e::render::vulkan::FrameResources& frame) {
            frame.descriptorAllocator->initPool();
        });
        environmentResources->build(environmentTexture, { 32, 32 }, { 8, 8 }, { 16, 16 });

        {
            bg2e::scene::Scene scene;
            auto root = std::make_shared<bg2e::scene::Node>("scene root");
            auto target = std::make_shared<bg2e::scene::Node>("bake target");
            auto environmentNode = std::make_shared<bg2e::scene::Node>("environment");
            environmentNode->addComponent(
                std::make_shared<bg2e::scene::EnvironmentComponent>("fixture_environment.hdr"));
            auto lightNode = std::make_shared<bg2e::scene::Node>("light");
            lightNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 1.0f));
            auto lightComponent = std::make_shared<bg2e::scene::LightComponent>();
            lightComponent->light().setShadowSamples(1);
            lightNode->addComponent(lightComponent);
            auto drawable = std::make_shared<bg2e::scene::Drawable>();
            drawable->setMesh(makeTriangleMesh());
            auto bakedMapSource = std::make_shared<bg2e::base::Texture>();
            bakedMapSource->setProceduralGenerator(new SolidRgbLightmapGenerator());
            drawable->material(0).setAoUVSet(1);
            drawable->load(&engine);
            auto renderMaterial = drawable->renderMaterial(0);
            renderMaterial->setUseTextureCache(false);
            drawable->material(0).setAoTexture(bakedMapSource);
            renderMaterial->setMaterialAttributes(drawable->material(0));
            renderMaterial->updateTextures();
            target->addComponent(std::make_shared<bg2e::scene::DrawableComponent>(drawable));
            root->addChild(target);
            root->addChild(environmentNode);
            root->addChild(lightNode);
            scene.setSceneRoot(root);
            verifyBakedRgbGBuffer(engine, engine.currentFrameResources(),
                frameDataBinding, objectDataBinding, *target->drawable());

            auto context = std::make_shared<bg2e::render::IntegratedBakerContext>(&engine, root.get());
            context->setEnvironmentResources(environmentResources.get());
            bg2e::render::LightmapSettings settings;
            settings.resolution = 64;
            settings.samplesPerPixel = 64;
            settings.maxRayDistance = 1.0f;
            auto baker = context->createBaker(target, settings);
            RenderDelegateFixture fixture(engine, *context, *baker, *environmentResources, root, target);

            // An isolated plane must remain neutral over the complete atlas.
            fixture.recordFrame(false);
            auto& firstFrame = engine.currentFrameResources();
            auto isolatedImage = baker->image();

            auto overhangNode = std::make_shared<bg2e::scene::Node>("overhanging occluder");
            auto overhangDrawable = std::make_shared<bg2e::scene::Drawable>();
            overhangDrawable->setMesh(makeOverhangMesh());
            overhangDrawable->load(&engine);
            overhangDrawable->renderMaterial(0)->materialAttributes().setAlbedo(bg2e::base::Color::Red());
            overhangNode->addComponent(std::make_shared<bg2e::scene::DrawableComponent>(overhangDrawable));
            root->addChild(overhangNode);

            bg2e::render::LightmapSettings rtaoWithShadows = settings;
            rtaoWithShadows.rtShadows = true;
            auto bakerRtaoWithShadows = context->createBaker(target, rtaoWithShadows);
            bg2e::render::LightmapSettings rtgi = settings;
            rtgi.mode = bg2e::render::LightmapMode::RTGI;
            rtgi.giBounces = 2;
            rtgi.samplesPerPixel = 8;
            auto bakerRtgi = context->createBaker(target, rtgi);
            bg2e::render::LightmapSettings rtgiWithShadows = rtgi;
            rtgiWithShadows.rtShadows = true;
            auto bakerRtgiWithShadows = context->createBaker(target, rtgiWithShadows);
            fixture.addBaker(*bakerRtaoWithShadows);
            fixture.addBaker(*bakerRtgi);
            fixture.addBaker(*bakerRtgiWithShadows);

            // Bake the overhang in the other slot after ordinary TLAS work.
            engine.nextFrame();
            fixture.recordFrame(true, true);
            auto& secondFrame = engine.currentFrameResources();
            auto overhangImage = baker->image();
            auto rtaoShadowImage = bakerRtaoWithShadows->image();
            auto rtgiImage = bakerRtgi->image();
            auto rtgiShadowImage = bakerRtgiWithShadows->image();
            VK_ASSERT(vkWaitForFences(engine.device().handle(), 1, &firstFrame.frameFence, VK_TRUE, UINT64_MAX));
            VK_ASSERT(vkWaitForFences(engine.device().handle(), 1, &secondFrame.frameFence, VK_TRUE, UINT64_MAX));
            const auto isolatedPixels = readLightmapImage(engine, *isolatedImage);
            require(std::all_of(isolatedPixels.begin(), isolatedPixels.end(),
                [](const glm::vec4& value) {
                    return value.r > 0.995f && value.g > 0.995f && value.b > 0.995f;
                }),
                "isolated plane produced non-neutral RTAO values");
            const auto rtaoPixels = readLightmapImage(engine, *overhangImage);
            const glm::vec4 underOverhang = sampleAtUv(rtaoPixels, settings.resolution, 0.36f, 0.36f);
            const glm::vec4 awayFromOverhang = sampleAtUv(rtaoPixels, settings.resolution, 0.15f, 0.15f);
            const glm::vec4 uncoveredTexel = sampleAtUv(rtaoPixels, settings.resolution, 0.75f, 0.75f);
            require(underOverhang.r + 0.08f < awayFromOverhang.r,
                "overhanging geometry did not selectively darken its mapped RTAO texels");
            require(uncoveredTexel.r > 0.995f && uncoveredTexel.g > 0.995f && uncoveredTexel.b > 0.995f,
                "uncovered UV atlas texel did not retain neutral RTAO visibility");
            const auto rtaoShadowPixels = readLightmapImage(engine, *rtaoShadowImage);
            const auto rtgiPixels = readLightmapImage(engine, *rtgiImage);
            const auto rtgiShadowPixels = readLightmapImage(engine, *rtgiShadowImage);
            const glm::vec4 rtaoWithShadow = sampleAtUv(rtaoShadowPixels, settings.resolution, 0.36f, 0.36f);
            const glm::vec4 gi = sampleAtUv(rtgiPixels, settings.resolution, 0.36f, 0.36f);
            const glm::vec4 giWithShadow = sampleAtUv(rtgiShadowPixels, settings.resolution, 0.36f, 0.36f);
            std::cout << "Lightmap combo samples: AO=(" << underOverhang.r << ", " << rtaoWithShadow.r
                      << "), GI=(" << gi.r << ", " << gi.g << ", " << gi.b << "), GI+shadow=("
                      << giWithShadow.r << ", " << giWithShadow.g << ", " << giWithShadow.b << ")" << std::endl;
            require(rtaoWithShadow.r < 0.05f && rtaoWithShadow.g < 0.05f && rtaoWithShadow.b < 0.05f,
                "fully shadowed RTAO texel was not near black");
            require(std::isfinite(gi.r) && std::isfinite(gi.g) && std::isfinite(gi.b) &&
                    (std::abs(gi.r - gi.g) > 0.02f || std::abs(gi.g - gi.b) > 0.02f),
                "RTGI did not retain a colored indirect multiplier");
            require(giWithShadow.r < 0.05f && giWithShadow.g < 0.05f && giWithShadow.b < 0.05f,
                "fully shadowed RTGI texel was not near black");

            // Revisit slot zero after its fence completes. The command-buffer
            // handle is recycled, so frame-number validation must reject stale
            // preparation before the new prepare call.
            engine.nextFrame();
            auto& recycledFrame = engine.currentFrameResources();
            VK_ASSERT(vkWaitForFences(engine.device().handle(), 1, &recycledFrame.frameFence, VK_TRUE, UINT64_MAX));
            VK_ASSERT(vkResetFences(engine.device().handle(), 1, &recycledFrame.frameFence));
            VK_ASSERT(vkResetCommandBuffer(recycledFrame.commandBuffer, 0));
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_ASSERT(vkBeginCommandBuffer(recycledFrame.commandBuffer, &beginInfo));
            requireThrows([&]() { baker->update(recycledFrame.commandBuffer, recycledFrame); },
                "recycled command buffer accepted stale frame preparation");
            context->prepareFrame(recycledFrame.commandBuffer, recycledFrame);
            baker->update(recycledFrame.commandBuffer, recycledFrame);
            VK_ASSERT(vkEndCommandBuffer(recycledFrame.commandBuffer));
            VkCommandBufferSubmitInfo commandInfo{};
            commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            commandInfo.commandBuffer = recycledFrame.commandBuffer;
            VkSubmitInfo2 submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = 1;
            submitInfo.pCommandBufferInfos = &commandInfo;
            VK_ASSERT(bg2e::render::vulkan::queueSubmit2(
                engine.device().graphicsQueue(), 1, &submitInfo, recycledFrame.frameFence));

            VK_ASSERT(vkWaitForFences(engine.device().handle(), 1, &recycledFrame.frameFence, VK_TRUE, UINT64_MAX));
        }

        environmentTexture.reset();
        engine.cleanup();
        SDL_DestroyWindow(window);
        SDL_Quit();
        std::cout << "Integrated baker scene, RTGI, shadow, and composition fixture passed" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Integrated baker scene, RTGI, shadow, and composition fixture failed: " << error.what() << std::endl;
        environmentResources.reset();
        if (engineInitialized)
        {
            engine.cleanup();
        }
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
}
