/*
 *    business grade graphic engine (bg2e engine)
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

// Manual stopped host; compile directly, no CMake registration.
#include "probe.hpp"
#include <bg2e/draw/all.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/geo/plane.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <set>

using namespace bg2e;

static void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

class CapturingTemporal : public draw::pipeline::TemporalAccumulator {
  public:
    using TemporalAccumulator::TemporalAccumulator;

    draw::pipeline::TemporalResult record(const draw::pipeline::TemporalInput& input) override
    {
        auto result = TemporalAccumulator::record(input);
        if (input.sceneFrameNumber == 31 && result.owner)
        {
            retained.emplace_back(result.owner);
        }
        return result;
    }

    std::vector<std::shared_ptr<gpu::Image>> retained;
};

static bool runS10(draw::Engine& engine, const std::filesystem::path& shaders,
    const std::filesystem::path& output)
{
    using namespace draw;
    RendererConfiguration configuration;
    configuration.outputExtent = configuration.internalExtent = {128,64};
    configuration.indirectLighting = IndirectLightingMode::AmbientOcclusion;
    pipeline::TemporalConfiguration temporalConfiguration;
    temporalConfiguration.shaderRoot = shaders;
    auto temporal = std::make_shared<CapturingTemporal>(temporalConfiguration);
    configuration.temporal = temporal;
    RendererDeferred renderer(shaders,2);
    renderer.initialize(engine,configuration);
    renderer.reflections()->setEnabled(false);
    renderer.ambientOcclusion()->setEnabled(true);
    renderer.temporal()->setMaxFrames(32);
    renderer.temporal()->setFreezeAtLimit(true);
    renderer.setCamera(glm::mat4(1),glm::mat4(1));
    require(!renderer.temporalSurfaceEnabled() && !renderer.temporal()->settings().adaptiveEnabled,
        "F0 requires replay and adaptation off");
    auto root = std::make_shared<scene::Node>("S10 F0");
    auto plane = [&](const char* name, float width, float height, float x, float z, base::Color color, bool transparent)
    {
        auto drawable = std::make_shared<Drawable>();
        auto mesh = std::shared_ptr<Drawable::Mesh>(geo::createPlane(width,height));
        const auto indices = mesh->indices;
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            mesh->indices.insert(mesh->indices.end(),{indices[i+2],indices[i+1],indices[i]});
        }
        mesh->submeshes[0] = {0,uint32_t(mesh->indices.size())};
        drawable->setMesh(mesh);
        drawable->material(0).setAlbedo(color);
        drawable->material(0).setIsTransparent(transparent);
        drawable->material(0).setIsSolid(!transparent);
        drawable->material(0).setRoughness(0.25f);
        drawable->load(&engine);
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(new scene::DrawableComponent(drawable));
        auto transform = glm::rotate(glm::mat4(1),-1.57079632679f,glm::vec3(1,0,0));
        transform[3] = glm::vec4(x,0,z,1);
        node->addComponent(new scene::TransformComponent(transform));
        root->addChild(node);
        return node;
    };
    plane("opaque wall",2,2,0,0.8f,{0.7f,0.7f,0.7f,1},false);
    auto near = plane("moving near glass",0.9f,1.4f,-0.15f,0.2f,{0.8f,0.15f,0.1f,0.5f},true);
    plane("stationary far glass",0.9f,1.4f,0.15f,0.4f,{0.1f,0.25f,0.8f,0.5f},true);
    renderer.attachScene(root);
    gpu::ImageDescription description;
    description.size = configuration.outputExtent;
    description.format = configuration.outputFormat;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    auto target = engine.device()->createImage(description);
    std::shared_ptr<gpu::Image> retained;
    std::vector<uint8_t> retainedBytes;
    std::vector<std::vector<uint8_t>> temporalBytes;
    std::set<uint64_t> layers;
    uint64_t generation = 0;
    bool changedPixels = false;
    std::vector<uint8_t> before;
    for (uint64_t sequence = 0; sequence < 145; ++sequence)
    {
        engine.device()->waitIdle(); // Stopped correctness fixture, not production profiling.
        if (sequence >= 32 && sequence < 48)
        {
            auto matrix = near->transform()->matrix();
            matrix[3].x = -0.15f+float(sequence-31)*0.025f;
            near->transform()->setMatrix(matrix);
        }
        renderer.prepareFrame();
        auto command = engine.device()->graphicsQueue().createCommandBuffer("S10 F0 correctness");
        command->begin();
        FrameContext frame{engine,*command,*target,description.size,sequence+1,uint32_t(sequence%2),0,sequence,0};
        renderer.update(frame);
        auto linear = renderer.recordLinear(frame);
        renderer.recordFinal(frame,*linear);
        require(!renderer.temporalSurfaceMetadata(frame.frameSlot).token,"F0 opaque metadata must be absent");
        std::set<uint64_t> current;
        for (const auto& item : renderer.queue().items())
        {
            if (item.drawMaterial->materialAttributes().isTransparent())
            {
                const auto layer = (uint64_t(item.objectId)<<32)|item.submeshIndex;
                current.insert(layer);
                require(!renderer.temporalSurfaceMetadata(frame.frameSlot,layer).token,"F0 transparent metadata must be absent");
            }
        }
        require(current.size() == 2,"S10 requires two distinct transparent layers");
        if (!sequence)
        {
            generation = renderer.historyGeneration();
            layers = current;
        }
        require(renderer.historyGeneration() == generation && layers == current,"Rigid glass motion must preserve global generation/layer identity");
        command->end();
        engine.device()->graphicsQueue().submit(command.get());
        engine.device()->waitIdle();
        if (sequence == 32)
        {
            std::vector<uint8_t> nextSlotBytes;
            retained->readPixels(nextSlotBytes,{},gpu::ImageLayout::ShaderReadOnly);
            require(nextSlotBytes == retainedBytes,"Linear output must survive until its own stopped slot reuse");
            retained.reset();
        }
        if (sequence == 31 || sequence == 47 || sequence == 144)
        {
            std::vector<uint8_t> pixels;
            target->readPixels(pixels,{},gpu::ImageLayout::ShaderReadOnly);
            require(pixels.size() == 128*64*4,"S10 output size");
            std::ofstream image(output/("s10-f0-"+std::to_string(sequence)+".ppm"),std::ios::binary);
            image << "P6\n128 64\n255\n";
            for (size_t i = 0; i < pixels.size(); i += 4)
            {
                image.write(reinterpret_cast<const char*>(pixels.data()+i),3);
            }
            if (sequence == 31)
            {
                before = pixels;
                retained = linear;
                retained->readPixels(retainedBytes,{},gpu::ImageLayout::ShaderReadOnly);
                for (const auto& image : temporal->retained)
                {
                    temporalBytes.emplace_back();
                    image->readPixels(temporalBytes.back(),{},gpu::ImageLayout::ShaderReadOnly);
                }
            }
            else if (sequence == 47)
            {
                changedPixels = pixels != before;
            }
        }
    }
    require(changedPixels,"Moving glass must change rendered pixels");
    for (size_t i = 0; i < temporal->retained.size(); ++i)
    {
        std::vector<uint8_t> after;
        temporal->retained[i]->readPixels(after,{},gpu::ImageLayout::ShaderReadOnly);
        require(after == temporalBytes.at(i),"Retained temporal output must remain immutable across slot reuse");
    }
    std::cout << "S10/F0 generation=" << generation << " layers=" << layers.size()
        << " AO=" << renderer.ambientOcclusion()->available() << " status=" << renderer.ambientOcclusion()->status()
        << " temporal=" << renderer.temporal()->available() << " upscaler=" << renderer.upscaler()->status() << std::endl;
    const bool temporalExercised = renderer.ambientOcclusion()->available() && renderer.temporal()->available();
    if (temporalExercised)
    {
        for (auto layer : layers)
        {
            require(renderer.temporal()->frameCount({pipeline::TemporalSignal::Visibility,0,layer}) == 32,"Transparent AO temporal key must converge nominally");
        }
    }
    renderer.cleanup();
    return temporalExercised;
}

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: native_host metal|vulkan probe|s10 SHADERS OUTPUT\n";
        return 2;
    }
    try
    {
        draw::EngineConfig config;
        config.backend = std::string(argv[1]) == "vulkan" ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
        config.applicationName = "temporal native validation";
        auto backend = gpu::Factory::acquireBackend(config.backend);
        draw::Engine engine;
        engine.init(gpu::Size2D{128,64},*backend,config);
        std::cout << "GPU=" << engine.physicalDevice()->properties()->name << " backend=" << argv[1] << std::endl;
        std::filesystem::create_directories(argv[4]);
        bool result;
        if (std::string(argv[2]) == "probe")
        {
            result = draw::tests::runTemporalProbe(engine,argv[3]);
            std::cout << "runTemporalProbe=" << (result ? "PASS" : "UNSUPPORTED") << std::endl;
        }
        else
        {
            result = runS10(engine,argv[3],argv[4]);
            std::cout << "S10/F0=" << (result ? "PASS scoped raster/temporal assertions" : "PARTIAL raster only; AO/temporal unavailable") << std::endl;
        }
        engine.cleanup();
        return result ? 0 : 3;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << std::endl;
        return 1;
    }
}
