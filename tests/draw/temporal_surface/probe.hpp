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

#pragma once
#include <bg2e/draw/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <set>

namespace bg2e::draw::tests {
class UnsupportedTemporalReceiver : public pipeline::GBuffer {
  public:
    using GBuffer::GBuffer;
};

// Stopped initialized Engine. No RT hardware required. All readbacks/waits are fixture-only.
// Returns false for unsupported metadata/diagnostic kernels, never a successful pass.
inline bool runTemporalSurfaceProbe(Engine& engine, const std::filesystem::path& shaderRoot, std::string* report = nullptr)
{
    using namespace pipeline;
    auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    UnsupportedTemporalReceiver custom(shaderRoot);
    TemporalSurfacePass unsupported(shaderRoot);
    unsupported.initialize(engine,custom,2);
    require(!unsupported.available() && !unsupported.status().empty(),"Custom receiver must explicitly retain fallback");
    unsupported.cleanup();
    if (report)
    {
        *report = "Metadata formats/kernels are queried by initialization; native pixel assertions pending";
    }
    RendererConfiguration configuration;
    configuration.outputExtent = configuration.internalExtent = {32,16};
    configuration.indirectLighting = IndirectLightingMode::IBL;
    configuration.upscaler = std::make_shared<ResamplingUpscaler>();
    RendererDeferred renderer(shaderRoot,2);
    renderer.initialize(engine,configuration);
    if (!renderer.temporalSurfaceAvailable() || !renderer.temporalDiagnosticsAvailable())
    {
        if (report)
        {
            *report = renderer.temporalSurfaceStatus()+"; "+renderer.temporal()->surfaceMetadataStatus();
        }
        renderer.cleanup();
        return false;
    }
    TemporalConfiguration consumerConfiguration;
    consumerConfiguration.shaderRoot = shaderRoot;
    consumerConfiguration.metadataShaderInterface = "draw.temporal.v2";
    TemporalAccumulator consumer(consumerConfiguration);
    consumer.initialize(*engine.device(),engine.backendType(),2);
    if (!consumer.diagnosticsAvailable())
    {
        if (report)
        {
            *report = consumer.surfaceMetadataStatus();
        }
        consumer.cleanup();
        renderer.cleanup();
        return false;
    }
    renderer.reflections()->setEnabled(false);
    renderer.setTemporalSurfaceEnabled(true);
    renderer.setTemporalDiagnosticView(TemporalDiagnosticView::Tokens);
    renderer.setCamera(glm::mat4(1),glm::mat4(1));
    auto root = std::make_shared<scene::Node>("temporal surface probe");
    auto addPlane = [&](const char* name, float width, float height, float z, bool split = false)
    {
        auto drawable = std::make_shared<Drawable>();
        drawable->setRayTracingEnabled(false);
        auto mesh = std::shared_ptr<Drawable::Mesh>(geo::createPlane(width,height));
        // Include both windings: the fixture isolates ownership rather than a backend front-face convention.
        const auto indices = mesh->indices;
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            mesh->indices.insert(mesh->indices.end(),{indices[i+2],indices[i+1],indices[i]});
        }
        mesh->submeshes[0] = {0,uint32_t(mesh->indices.size())};
        if (split)
        {
            mesh->indices = {indices[0],indices[1],indices[2],indices[2],indices[1],indices[0],
                indices[3],indices[4],indices[5],indices[5],indices[4],indices[3]};
            mesh->submeshes = {{0,6},{6,6}};
        }
        drawable->setMesh(mesh);
        drawable->load(&engine);
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(new scene::DrawableComponent(drawable));
        root->addChild(node);
        auto world = glm::rotate(glm::mat4(1),-1.57079632679f,glm::vec3(1,0,0));
        world[3].z = z;
        node->addComponent(new scene::TransformComponent(world));
        return std::pair{node,drawable};
    };
    auto [wallNode,wall] = addPlane("wall",2,2,0.6f);
    auto [frontNode,front] = addPlane("foreground",0.75f,1.0f,0.599f,true); // Close parallel receivers, equal normals.
    renderer.attachScene(root);
    gpu::ImageDescription targetDescription;
    targetDescription.size = configuration.outputExtent;
    targetDescription.format = configuration.outputFormat;
    targetDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    auto target = engine.device()->createImage(targetDescription);
    struct Snapshot {
        TemporalSurfaceMetadata metadata;
        std::shared_ptr<gpu::Image> acceptance;
        std::shared_ptr<gpu::Image> diagnostic;
        TemporalSurfaceMetadata transparent;
        uint64_t layer = 0;
    };
    gpu::ImageDescription normalDescription;
    normalDescription.size = configuration.internalExtent;
    normalDescription.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    normalDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    std::vector<std::array<uint16_t,4>> normals(32*16,{0x3800,0x3800,0x3c00,0});
    auto normal = engine.device()->createImage(normalDescription);
    normal->upload({normals.data(),normals.size()*8,32*8,0,0});
    std::vector<std::shared_ptr<gpu::Image>> signals;
    std::vector<std::shared_ptr<gpu::CommandBuffer>> commands;
    for (auto value : {uint16_t(0x3400),uint16_t(0x3a00)})
    {
        auto signalDescription = normalDescription;
        signalDescription.format = gpu::PixelFormat::R16_SFLOAT;
        auto signal = engine.device()->createImage(signalDescription);
        const std::vector<uint16_t> values(32*16,value);
        signal->upload({values.data(),values.size()*2,32*2,0,0});
        signals.push_back(signal);
    }
    uint64_t sequence = 0, serial = 0;
    bool captureReceiverDepth = false;
    auto record = [&](bool submit = true)
    {
        renderer.prepareFrame();
        const uint64_t frameSerial = ++serial, scene = sequence++;
        const uint32_t slot = uint32_t(scene%2);
        auto signal = signals.at(size_t(scene%2));
        auto command = engine.device()->graphicsQueue().createCommandBuffer("temporal surface fixture");
        command->begin();
        FrameContext frame{engine,*command,*target,targetDescription.size,frameSerial,slot,0,scene,0};
        renderer.update(frame);
        const auto linear = renderer.recordLinear(frame);
        if (captureReceiverDepth)
        {
            renderer.recordFinal(frame,*linear);
        }
        Snapshot snapshot;
        snapshot.metadata = renderer.temporalSurfaceMetadata(slot);
        consumer.beginFrame(slot,frameSerial,scene);
        if (snapshot.metadata.token)
        {
            SurfaceInputs surface{SurfaceProfile::CameraDepth,normal.get(),snapshot.metadata.depth.get(),nullptr,glm::mat4(1)};
            TemporalView view;
            view.reproject = false;
            consumer.record({*command,slot,frameSerial,scene,renderer.historyGeneration(),{},*signal,surface,view,nullptr,1,
                snapshot.metadata,true});
            snapshot.acceptance = consumer.recordDiagnostic(*command,slot,frameSerial,{},snapshot.metadata,TemporalDiagnosticView::LocalHistory);
        }
        snapshot.diagnostic = renderer.temporal()->recordDiagnostic(*command,slot,frameSerial,{},snapshot.metadata,TemporalDiagnosticView::Tokens);
        for (const auto& item : renderer.queue().items())
        {
            if (item.nodeId == frontNode->identifier() && item.drawMaterial->materialAttributes().isTransparent())
            {
                snapshot.layer = (uint64_t(item.objectId)<<32)|item.submeshIndex;
                snapshot.transparent = renderer.temporalSurfaceMetadata(slot,snapshot.layer);
            }
        }
        command->end();
        if (submit)
        {
            engine.device()->graphicsQueue().submit(command.get());
        }
        else
        {
            command->recordingCompletion()->abandon();
        }
        commands.push_back(command);
        return snapshot;
    };
    using Pixel = std::array<uint16_t,4>;
    const Pixel black{0,0,0,0x3c00};
    auto read = [&](const std::shared_ptr<gpu::Image>& image)
    {
        require(bool(image),"Expected supported GPU diagnostic output");
        std::vector<uint8_t> bytes;
        image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        require(bytes.size() == 32*16*sizeof(Pixel),"Surface diagnostic readback extent differs");
        std::vector<Pixel> pixels(32*16);
        std::memcpy(pixels.data(),bytes.data(),bytes.size());
        return pixels;
    };
    const auto first = record();
    const auto stable = record(); // Two slots admitted in order before any fixture wait/readback.
    engine.device()->waitIdle();
    const auto firstPixels = read(first.diagnostic), stablePixels = read(stable.diagnostic);
    require(std::all_of(firstPixels.begin(),firstPixels.end(),[&](const auto& p) { return p == black; }),
        "First scene has no static predecessor and must emit zero tokens");
    const Pixel wallColor = stablePixels[0];
    require(wallColor != black,"Static wall must become valid after a submitted predecessor");
    const size_t center = 8*32+16;
    require(stablePixels[center] != wallColor && stablePixels[center] != black,
        "Nearby parallel foreground must have an independent token despite equal normals/depth tolerance");
    std::set<Pixel> foregroundColors;
    for (const auto& pixel : stablePixels)
    {
        if (pixel != black && pixel != wallColor)
        {
            foregroundColors.insert(pixel);
        }
    }
    require(foregroundColors.size() == 2,"Two submeshes of one node must rasterize independent stable tokens");
    const Pixel green{0,0x3c00,0,0x3c00}, red{0x3c00,0,0,0x3c00};
    const auto converged = record();
    engine.device()->waitIdle();
    const auto convergedAcceptance = read(converged.acceptance);
    require(convergedAcceptance[0] == green && convergedAcceptance[center] == green,
        "Static wall and foreground must accept actual metadata history after warmup");
    auto moved = frontNode->transform()->matrix();
    moved[3].x = 0.65f;
    frontNode->transform()->setMatrix(moved);
    const auto generation = renderer.historyGeneration();
    const auto translated = record();
    engine.device()->waitIdle();
    const auto translatedPixels = read(translated.diagnostic);
    require(renderer.historyGeneration() == generation && translatedPixels[center] == wallColor &&
        translatedPixels[0] == wallColor,"Moving foreground must expose the original wall without resetting its neighbors globally");
    const auto translatedAcceptance = read(translated.acceptance);
    require(translatedAcceptance[0] == green && translatedAcceptance[center] == red,
        "Unchanged wall must keep history while newly exposed wall rejects the previous foreground");
    require(std::count(translatedPixels.begin(),translatedPixels.end(),black) > 0,
        "Moved foreground must emit invalid zero tokens locally");
    record(); // Recover stationary foreground after motion.
    engine.device()->waitIdle();
    require(read(stable.diagnostic) == stablePixels && stable.metadata.token != translated.metadata.token,
        "Retained earlier metadata/diagnostic outputs must survive two-slot reuse");
    record(false); // Publish a scene recording that is deliberately never submitted.
    const auto afterAbandon = record();
    engine.device()->waitIdle();
    const auto abandonedPixels = read(afterAbandon.diagnostic);
    require(std::all_of(abandonedPixels.begin(),abandonedPixels.end(),[&](const auto& p) { return p == black; }),
        "Abandoned scene cannot supply static correspondence");
    uint32_t oldForegroundId = 0;
    for (const auto& item : renderer.queue().items())
    {
        if (item.nodeId == frontNode->identifier() && item.submeshIndex == 0)
        {
            oldForegroundId = item.objectId;
        }
    }
    auto [insertedNode,insertedDrawable] = addPlane("inserted before existing extraction",0.1f,0.1f,0.9f);
    auto outside = insertedNode->transform()->matrix();
    outside[3].x = 4;
    insertedNode->transform()->setMatrix(outside);
    root->removeChild(wallNode);
    root->removeChild(frontNode);
    root->addChild(wallNode);
    root->addChild(frontNode);
    const auto beforeInsertion = renderer.historyGeneration();
    record();
    const auto afterInsertion = record();
    engine.device()->waitIdle();
    uint32_t newForegroundId = 0;
    for (const auto& item : renderer.queue().items())
    {
        if (item.nodeId == frontNode->identifier() && item.submeshIndex == 0)
        {
            newForegroundId = item.objectId;
        }
    }
    require(newForegroundId != oldForegroundId && read(afterInsertion.diagnostic)[0] == wallColor &&
        renderer.historyGeneration() > beforeInsertion,
        "Insertion must shift extraction IDs while preserving wall identity and conservative membership reset");
    // Alpha texture creates genuine cutout holes; compare against the original full receiver below.
    moved[3].x = 0;
    moved[3].z = 0.2f; // Distinct UNORM depth values for actual GBuffer coverage comparison.
    frontNode->transform()->setMatrix(moved);
    gpu::ImageDescription alphaDescription;
    alphaDescription.size = {2,2};
    alphaDescription.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    alphaDescription.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    const uint8_t alphaPixels[]{255,255,255,0,255,255,255,255,255,255,255,0,255,255,255,255};
    auto alphaImage = engine.device()->createImage(alphaDescription);
    alphaImage->upload({alphaPixels,sizeof(alphaPixels),8,0,0});
    auto alphaSettings = std::make_shared<base::Texture>();
    alphaSettings->setMagFilter(base::Texture::FilterNearest);
    alphaSettings->setMinFilter(base::Texture::FilterNearest);
    auto alpha = std::make_shared<Texture>(&engine);
    alpha->load(alphaSettings,alphaImage);
    for (uint32_t submesh = 0; submesh < front->submeshesCount(); ++submesh)
    {
        front->drawMaterial(submesh)->setTexture(TextureSlot::Albedo,alpha);
        front->material(submesh).setAlphaCutoff(0.5f);
    }
    record();
    captureReceiverDepth = true;
    renderer.finalSettings().display.x = uint32_t(FinalDebugView::Depth);
    const auto holes = record();
    captureReceiverDepth = false;
    engine.device()->waitIdle();
    const auto holePixels = read(holes.diagnostic);
    const Pixel newWallColor = holePixels[0];
    std::vector<uint8_t> actualDepth;
    target->readPixels(actualDepth,{},gpu::ImageLayout::ShaderReadOnly);
    require(actualDepth.size() == 32*16*4,"Actual GBuffer depth diagnostic extent differs");
    for (size_t i = 0; i < holePixels.size(); ++i)
    {
        const int expected = holePixels[i] == newWallColor ? 153 : 51; // .6 wall, .2 foreground.
        require(holePixels[i] != black && std::abs(int(actualDepth[i*4])-expected) <= 2,
            "Metadata alpha coverage must match actual GBuffer receiver depth at each pixel");
    }
    size_t wallInside = 0, frontInside = 0;
    for (uint32_t y = 5; y < 11; ++y)
    {
        for (uint32_t x = 12; x < 20; ++x)
        {
            wallInside += holePixels[y*32+x] == newWallColor;
            frontInside += holePixels[y*32+x] != newWallColor && holePixels[y*32+x] != black;
        }
    }
    require(wallInside && frontInside,"Alpha-cutout holes must expose wall tokens beside covered foreground tokens");
    for (uint32_t submesh = 0; submesh < front->submeshesCount(); ++submesh)
    {
        front->material(submesh).setIsTransparent(true);
        front->material(submesh).setIsSolid(false);
    }
    record();
    const auto transparent = record();
    engine.device()->waitIdle();
    require(transparent.transparent.token && transparent.transparent.depth &&
        transparent.transparent.token != transparent.metadata.token && transparent.transparent.depth != transparent.metadata.depth,
        "Transparent receiver must own a separate full-extent pair seeded from opaque depth");
    renderer.setTemporalDiagnosticLayer(transparent.layer);
    const auto transparentView = record();
    engine.device()->waitIdle();
    // Read layer diagnostic explicitly; renderer output uses the same exact-layer selection.
    auto command = engine.device()->graphicsQueue().createCommandBuffer("transparent metadata diagnostic");
    command->begin();
    const uint32_t lastSlot = uint32_t((sequence-1)%2);
    auto transparentDiagnostic = renderer.temporal()->recordDiagnostic(*command,lastSlot,serial,{},transparentView.transparent,TemporalDiagnosticView::Tokens);
    command->end();
    engine.device()->graphicsQueue().submit(command.get());
    engine.device()->waitIdle();
    const auto transparentPixels = read(transparentDiagnostic);
    require(transparentPixels[0] == black && std::any_of(transparentPixels.begin(),transparentPixels.end(),[&](const auto& p) { return p != black; }),
        "Transparent token must clear outside coverage, preserve holes and pass private opaque-depth occlusion");
    const auto opaquePixels = read(transparentView.diagnostic);
    require(opaquePixels[0] != black && opaquePixels[center] == opaquePixels[0],
        "Transparent replay must not overwrite the preserved opaque wall domain");
    moved[3].z = 0.601f; // Transparent receiver behind the opaque wall must fail private depth.
    frontNode->transform()->setMatrix(moved);
    record();
    const auto occluded = record();
    engine.device()->waitIdle();
    auto hiddenCommand = engine.device()->graphicsQueue().createCommandBuffer("occluded transparent metadata");
    hiddenCommand->begin();
    auto hiddenDiagnostic = renderer.temporal()->recordDiagnostic(*hiddenCommand,uint32_t((sequence-1)%2),serial,{},
        occluded.transparent,TemporalDiagnosticView::Tokens);
    hiddenCommand->end();
    engine.device()->graphicsQueue().submit(hiddenCommand.get());
    engine.device()->waitIdle();
    const auto hiddenPixels = read(hiddenDiagnostic);
    require(std::all_of(hiddenPixels.begin(),hiddenPixels.end(),[&](const auto& p) { return p == black; }),
        "Transparent private depth must reject a receiver behind preserved opaque depth");
    renderer.setTemporalSurfaceEnabled(false);
    const auto disabled = record();
    engine.device()->waitIdle();
    require(!disabled.metadata.token && !disabled.metadata.depth && !disabled.diagnostic,
        "Disabled metadata must retain Phase 0 and publish no receiver pair");
    if (report)
    {
        *report = "Passed fixture assertions: R32_UINT sampled/color metadata, D32 sampled private depth, R16 scalar history, RGBA16 diagnostics; native backend selected by host";
    }
    consumer.cleanup();
    renderer.cleanup();
    return true;
}
}
