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

#include <bg2e/bake/StandaloneBakeBatch.hpp>
#include <bg2e/bake/StandaloneBakerContext.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/environment/EnvironmentResources.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/Node.hpp>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::bake {
StandaloneBakeBatch::Result StandaloneBakeBatch::run(draw::Engine& engine,
    const StandaloneBakeSceneAssembler::Assembly& assembly, const std::filesystem::path& outputDirectory,
    const Options& requestedOptions, ProgressCallback onProgress)
{
    const Options options = requestedOptions; // Freeze settings and job callbacks before user code.
    options.settings.validate();
    if (!assembly.scene || !assembly.scene->rootNode() || !assembly.targetRoot || assembly.targets.empty() ||
        assembly.targets.size() > std::numeric_limits<uint32_t>::max())
    {
        throw std::invalid_argument("Standalone batch requires a complete nonempty assembly");
    }
    const auto sceneOwner = assembly.scene;
    const auto root = sceneOwner->rootNode()->shared_from_this();
    Result result;
    if (options.current && !options.current())
    {
        result.cancelled = true;
        return result;
    }
    // Do not repeat custom resource hooks on an already realized assembly.
    bool deferred = false;
    std::vector<std::shared_ptr<draw::Drawable>> prepared;
    std::vector<std::shared_ptr<draw::Drawable::Mesh>> preparedMeshes;
    for (const auto& target : assembly.targets)
    {
        const auto component = target.node ? target.node->drawable() : nullptr;
        const auto drawable = component ? component->drawable() : nullptr;
        if (!drawable)
        {
            throw std::invalid_argument("Standalone batch has no prepared draw geometry");
        }
        prepared.push_back(drawable);
        preparedMeshes.push_back(drawable->mesh());
        deferred = deferred || !drawable->isLoaded();
    }
    if (deferred)
    {
        assembly.targetRoot->loadResources(engine);
    }
    std::vector<LightmapOutputWriter::Target> targets;
    for (const auto& t : assembly.targets)
    {
        const auto index = targets.size();
        if (!t.node || t.node->sceneRoot() != assembly.scene->rootNode())
        {
            throw std::invalid_argument("Standalone batch target is detached from its assembly");
        }
        if (!t.node->drawable() || t.node->drawable()->drawable() != prepared[index] ||
            !prepared[index]->isLoaded() || prepared[index]->engine() != &engine ||
            prepared[index]->mesh() != preparedMeshes[index])
        {
            throw std::logic_error("Deferred resources replaced or incompletely realized prepared geometry");
        }
        targets.push_back({t.outputIdentity, t.inputSourcePath, t.node});
    }
    auto inputs = assembly.sourcePaths;
    inputs.insert(inputs.end(), options.protectedInputs.begin(), options.protectedInputs.end());
    inputs.push_back(assembly.contextSourcePath);
    inputs.push_back(assembly.inputSourcePath);
    LightmapOutputWriter writer(engine, outputDirectory, options.settings, options.byteFormat);
    writer.setOverwriteExisting(options.overwriteOutputs);
    writer.preflight(targets, options.writeModelCopies, inputs, options.resolveDependencies);
    StandaloneBakerContext context(engine);
    context.initialize(options.shaderRoot);
    context.prepareScene(root, options.environment);
    const auto environment = context.configuration().environment;
    const auto images = std::array{environment->sky, environment->irradiance,
        environment->prefilteredSpecular, environment->brdf};
    const auto sampler = environment->sampler;
    const auto brdfSampler = environment->brdfSampler;
    const auto blur = environment->skyboxBlur;
    const auto visible = environment->skyboxVisible;
    std::array<uint64_t, 4> generations;
    for (size_t i = 0; i < images.size(); ++i)
    {
        generations[i] = images[i]->bindingGeneration();
    }
    const auto valid = [&]
    {
        if (options.current && !options.current())
        {
            return false;
        }
        if (sceneOwner->rootNode() != root.get())
        {
            throw std::runtime_error("Standalone batch scene root changed");
        }
        writer.validateSources();
        if (images != std::array{environment->sky, environment->irradiance,
                environment->prefilteredSpecular, environment->brdf} ||
            sampler != environment->sampler || brdfSampler != environment->brdfSampler ||
            blur != environment->skyboxBlur || visible != environment->skyboxVisible)
        {
            throw std::runtime_error("Standalone batch environment changed");
        }
        for (size_t i = 0; i < images.size(); ++i)
        {
            if (images[i]->bindingGeneration() != generations[i])
            {
                throw std::runtime_error("Standalone batch environment allocation changed");
            }
        }
        return true;
    };
    const auto notify = [&](uint32_t index, uint32_t completed)
    {
        const bool proceed = !onProgress || onProgress({index, static_cast<uint32_t>(targets.size()),
            completed, options.settings.accumulationFrames()});
        // Even a cancellation callback may have invalidated its source document.
        return valid() && proceed;
    };
    for (uint32_t i = 0; i < targets.size(); ++i)
    {
        if (!notify(i, 0))
        {
            result.cancelled = true;
            return result;
        }
        LightmapBakerConfiguration configuration;
        configuration.target = targets[i].node;
        configuration.settings = std::make_shared<LightmapSettings>(options.settings);
        auto baker = context.createBaker(std::move(configuration));
        while (baker->progress().state != BakeState::Complete)
        {
            baker->update();
            if (!notify(i, baker->progress().completedSamples))
            {
                baker->cancel();
                result.cancelled = true;
                return result;
            }
        }
        if (!valid())
        {
            result.cancelled = true;
            return result;
        }
        writer.write(baker->readPixels(), targets[i].identity);
        baker->cleanup();
        baker.reset(); // Release the exclusive context target before creating another.
    }
    if (!valid())
    {
        result.cancelled = true;
        return result;
    }
    // Allocate/copy the result before publication so allocation failures cannot
    // turn a successfully published batch into an exception reported as failure.
    result.outputs = writer.outputs();
    result.manifestPath = writer.manifestPath();
    writer.commit();
    result.bakedCount = static_cast<uint32_t>(targets.size());
    return result;
}
}
