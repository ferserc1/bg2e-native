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
#include <bg2e/scene/LightComponent.hpp>
#include <stdexcept>

namespace bg2e::draw::tests {
class SignatureDenoiseProbe : public pipeline::DenoiseFilter {
  public:
    using DenoiseFilter::DenoiseFilter;
    uint32_t effectiveRadius = 2;

    uint32_t kernelRadius() const override
    {
        return effectiveRadius;
    }
};

// Stopped initialized Engine and paired native shaders. Compares full bytes and
// reset decisions through the actual renderer, without rendering image output.
inline bool runTemporalInvalidationProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    const auto usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled;
    if (!engine.device()->formatCapabilities(gpu::PixelFormat::R8G8B8A8_UNORM).supports(usage))
    {
        return false;
    }
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    RendererConfiguration configuration;
    configuration.outputExtent = configuration.internalExtent = {8,8};
    configuration.indirectLighting = IndirectLightingMode::IBL;
    pipeline::DenoiseConfiguration denoiseConfiguration;
    denoiseConfiguration.shaderRoot = shaderRoot;
    auto denoise = std::make_shared<SignatureDenoiseProbe>(denoiseConfiguration);
    configuration.denoise = denoise;
    RendererDeferred renderer(shaderRoot, 2);
    renderer.initialize(engine, configuration);
    renderer.setTemporalSignatureValidationEnabled(true);
    renderer.reflections()->setEnabled(false);
    renderer.setCamera(glm::mat4(1), glm::mat4(1));
    auto root = std::make_shared<scene::Node>("signature probe");
    auto a = std::make_shared<Drawable>();
    a->setRayTracingEnabled(false);
    a->setMesh(geo::createPlane(1,1));
    a->load(&engine);
    auto b = a->clone();
    auto first = std::make_shared<scene::Node>("independent A");
    auto second = std::make_shared<scene::Node>("independent B");
    first->addComponent(new scene::DrawableComponent(a));
    second->addComponent(new scene::DrawableComponent(b));
    root->addChild(first);
    root->addChild(second);
    auto light = std::make_shared<scene::LightComponent>();
    auto lamp = std::make_shared<scene::Node>("signature light");
    lamp->addComponent(light);
    root->addChild(lamp);
    renderer.attachScene(root);
    gpu::ImageDescription output;
    output.size = configuration.outputExtent;
    output.format = configuration.outputFormat;
    output.usage = usage;
    auto target = engine.device()->createImage(output);
    uint64_t serial = 0;
    const auto update = [&]()
    {
        renderer.prepareFrame();
        const auto sequence = serial++;
        engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            FrameContext frame{engine,*command,*target,output.size,sequence,uint32_t(sequence%2),0,sequence,0};
            renderer.update(frame);
        });
        // Fingerprint queries must not consume changes before the next update.
        const auto cached = renderer.sceneFingerprint();
        renderer.setTemporalSignatureReuseEnabled(false);
        const auto reference = renderer.sceneFingerprint();
        renderer.setTemporalSignatureReuseEnabled(true);
        require(cached == reference, "Cached fingerprints must retain the exact full serialization format");
        return renderer.historyGeneration();
    };
    const auto warm = update();
    require(update() == warm && update() == warm, "Static scenes must not lose history across slots");
    auto view = glm::mat4(1);
    view[3].x = 0.05f;
    renderer.setCamera(view, glm::mat4(1));
    require(update() == warm, "Camera view remains outside the global signature; per-history camera rejection stays separate");
    auto projection = glm::mat4(1);
    projection[0][0] = 1.1f;
    renderer.setCamera(view, projection);
    auto generation = update();
    require(generation > warm, "Projection changes must preserve the conservative global reset");
    auto moved = glm::mat4(1);
    moved[3].x = 0.1f;
    a->setTransform(moved);
    require(update() > generation, "Object motion must still invalidate every affected history");
    generation = renderer.historyGeneration();
    const auto independentRoughness = b->material().roughness();
    a->material().setRoughness(0.123f);
    require(update() > generation && b->material().roughness() == independentRoughness,
        "Material edits must reset globally without merging independent material identities");
    generation = renderer.historyGeneration();
    a->material().setAlphaCutoff(AlphaTestCutoffThreshold * 0.5f);
    require(update() == generation, "Authored edits with identical effective packed values must preserve the prior reset decision");
    a->material().setAlphaCutoff(AlphaTestCutoffThreshold);
    require(update() > generation, "Opacity threshold crossings must invalidate material-dependent histories");
    generation = renderer.historyGeneration();
    a->drawMaterial(0)->albedoTexture()->reload();
    require(update() > generation, "Texture replacement must invalidate every affected effect");
    generation = renderer.historyGeneration();
    a->material().setIsTransparent(true);
    require(update() > generation, "Transparent classification and layer identities must remain observable");
    generation = renderer.historyGeneration();
    root->removeChild(first);
    root->addChild(first);
    require(update() > generation, "Node/submesh correspondence and extraction-order changes must reset conservatively");
    generation = renderer.historyGeneration();
    root->removeChild(second);
    require(update() > generation, "Object removal must invalidate membership");
    generation = renderer.historyGeneration();
    root->addChild(second);
    require(update() > generation, "Object addition must invalidate membership");
    generation = renderer.historyGeneration();
    light->light().setIntensity(light->light().intensity()+1);
    require(update() > generation, "Lighting changes must retain global GI/reflection invalidation");
    generation = renderer.historyGeneration();
    denoise->effectiveRadius = 3;
    const auto beforeUpdate = renderer.sceneFingerprint();
    require(!beforeUpdate.empty() && update() > generation,
        "Virtual effective getters and out-of-band fingerprints must not hide invalidation from update");
    generation = renderer.historyGeneration();
    renderer.setTemporalSignatureReuseEnabled(false);
    // The update helper restores reuse for fingerprint comparison only after update.
    require(update() == generation, "Switching to the full baseline must preserve accepted history decisions");
    renderer.invalidateHistories();
    const auto explicitReset = renderer.historyGeneration();
    require(explicitReset > generation && update() == explicitReset, "Explicit global resets must keep their original semantics");
    renderer.cleanup();
    return true;
}
}
