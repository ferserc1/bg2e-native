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

#include <bg2e/render/StandaloneBakeBatch.hpp>

#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/StandaloneBakerContext.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::render {
namespace {

struct ValidTarget {
    const StandaloneBakeSceneAssembler::Target* assemblyTarget = nullptr;
    std::shared_ptr<scene::Drawable> drawable;
};

void validateBatchOptions(const StandaloneBakeBatch::Options& options)
{
    const auto& settings = options.lightmapSettings;
    if (settings.resolution == 0 || settings.accumulationFrames == 0 || settings.samplesPerPixel == 0)
    {
        throw std::invalid_argument(
            "StandaloneBakeBatch: resolution, accumulationFrames and samplesPerPixel must be positive");
    }
    if (settings.samplesPerPixel > static_cast<uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument("StandaloneBakeBatch: samplesPerPixel exceeds the RTAO shader limit");
    }
    if (settings.mode != LightmapMode::RTAO && settings.mode != LightmapMode::RTGI)
    {
        throw std::invalid_argument("StandaloneBakeBatch: unsupported lightmap mode");
    }
    if (settings.cpuFormat != LightmapPixelFormat::RGB8 &&
        settings.cpuFormat != LightmapPixelFormat::RGB32F)
    {
        throw std::invalid_argument("StandaloneBakeBatch: unsupported CPU pixel format");
    }
    if (settings.mode == LightmapMode::RTGI && settings.giBounces == 0)
    {
        throw std::invalid_argument("StandaloneBakeBatch: giBounces must be positive for RTGI");
    }
    if (settings.mode == LightmapMode::RTGI &&
        (!std::isfinite(settings.maxRayDistance) || settings.maxRayDistance <= 0.0f))
    {
        throw std::invalid_argument("StandaloneBakeBatch: maxRayDistance must be finite and positive for RTGI");
    }
    if (settings.mode == LightmapMode::RTGI &&
        (!std::isfinite(settings.giRayBias) || settings.giRayBias < 0.0f ||
         settings.giRayBias >= settings.maxRayDistance))
    {
        throw std::invalid_argument(
            "StandaloneBakeBatch: giRayBias must be non-negative and smaller than maxRayDistance");
    }
    if (!std::isfinite(settings.exposureEV) || settings.exposureEV < -16.0f || settings.exposureEV > 16.0f)
    {
        throw std::invalid_argument("StandaloneBakeBatch: exposureEV must be finite and within [-16, 16]");
    }
    if (settings.dilationPixels == 0 || settings.dilationPixels > 32)
    {
        throw std::invalid_argument("StandaloneBakeBatch: dilationPixels must be in [1, 32]");
    }
}

std::shared_ptr<scene::Drawable> requireTargetDrawable(
    const StandaloneBakeSceneAssembler::Target& target,
    bool requireGpuLoaded = false)
{
    if (!target.node || !target.node->drawable())
    {
        throw std::invalid_argument(
            "StandaloneBakeBatch: target '" + target.outputIdentity + "' does not directly contain a Drawable");
    }
    auto drawable = target.node->drawable()->drawable();
    if (!drawable || !drawable->mesh() || (requireGpuLoaded && !drawable->isLoaded()))
    {
        throw std::invalid_argument(
            "StandaloneBakeBatch: target '" + target.outputIdentity + "' does not have the required standard Drawable resources");
    }
    return drawable;
}

void generateUv2Atlas(
    const ValidTarget& target,
    const StandaloneBakeBatch::Options& options)
{
    const geo::Uv2AtlasOptions atlasOptions{
        options.lightmapSettings.resolution,
        options.uv2PaddingPixels
    };

    auto mesh = target.drawable->mesh();
    if (!mesh)
    {
        throw std::runtime_error("target CPU mesh disappeared during UV2 generation");
    }
    try
    {
        geo::GenerateUv2AtlasModifier modifier(mesh.get(), atlasOptions);
        modifier.apply();
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error(error.what());
    }
    const auto validation = geo::UvAtlasValidator::validate(*mesh, 1);
    if (!validation.valid)
    {
        throw std::runtime_error("generated UV2 atlas is unusable: " + validation.message);
    }
}

void loadTargetDrawables(
    Engine* engine,
    const std::vector<StandaloneBakeSceneAssembler::Target>& targets)
{
    for (const auto& target : targets)
    {
        auto drawable = requireTargetDrawable(target);
        if (!drawable->isLoaded())
        {
            drawable->load(engine);
        }
        if (!drawable->isLoaded())
        {
            throw std::runtime_error(
                "StandaloneBakeBatch: failed to GPU-load target '" + target.outputIdentity + "'");
        }
    }
    engine->device().waitIdle();
}

}

StandaloneBakeBatch::Result StandaloneBakeBatch::run(
    Engine* engine,
    const StandaloneBakeSceneAssembler::Assembly& assembly,
    const std::filesystem::path& outputDirectory,
    db::ImageFormat imageFormat,
    const Options& options,
    ProgressCallback onProgress,
    WarningCallback onWarning)
{
    if (!engine)
    {
        throw std::invalid_argument("StandaloneBakeBatch: engine must not be null");
    }
    if (!assembly.scene || !assembly.scene->rootNode())
    {
        throw std::invalid_argument("StandaloneBakeBatch: assembly must contain a scene root");
    }
    if (!engine->rayTracingSupported())
    {
        throw std::runtime_error("StandaloneBakeBatch requires ray tracing support");
    }
    if (db::canonicalImageExtension(imageFormat).empty())
    {
        throw std::invalid_argument("StandaloneBakeBatch: unsupported image format");
    }
    validateBatchOptions(options);

    Result result;
    std::vector<ValidTarget> validTargets;
    validTargets.reserve(assembly.targets.size());
    for (const auto& target : assembly.targets)
    {
        auto drawable = requireTargetDrawable(target);
        if (options.generateUv2 && drawable->isLoaded())
        {
            throw std::invalid_argument(
                "StandaloneBakeBatch: UV2 generation requires CPU-only target assembly; "
                "call the assembler with loadTargetGpuResources=false");
        }
        if (target.node->sceneRoot() != assembly.scene->rootNode())
        {
            throw std::invalid_argument(
                "StandaloneBakeBatch: target '" + target.outputIdentity + "' is detached from the assembled scene");
        }

        if (options.generateUv2)
        {
            validTargets.push_back({ &target, std::move(drawable) });
            continue;
        }

        const auto mesh = drawable->mesh();
        const auto validation = geo::UvAtlasValidator::validate(*mesh, 1);
        if (!validation.valid)
        {
            result.skippedTargets.push_back({ target.outputIdentity, validation.message });
            if (onWarning)
            {
                onWarning(result.skippedTargets.back());
            }
            continue;
        }
        validTargets.push_back({ &target, std::move(drawable) });
    }
    result.skippedCount = static_cast<uint32_t>(result.skippedTargets.size());

    if (validTargets.empty())
    {
        return result;
    }

    db::LightmapOutputWriter outputWriter(outputDirectory, imageFormat);
    outputWriter.setOverwriteExisting(options.overwriteOutputs);
    std::vector<db::LightmapOutputWriter::Target> outputTargets;
    outputTargets.reserve(validTargets.size());
    for (const auto& target : validTargets)
    {
        outputTargets.push_back({
            target.assemblyTarget->outputIdentity,
            target.assemblyTarget->inputSourcePath,
            target.assemblyTarget->node
        });
    }

    std::vector<std::filesystem::path> protectedInputPaths{
        assembly.contextSourcePath,
        assembly.inputSourcePath
    };
    protectedInputPaths.reserve(protectedInputPaths.size() + assembly.targets.size());
    for (const auto& target : assembly.targets)
    {
        protectedInputPaths.push_back(target.inputSourcePath);
    }
    const auto outputPaths = outputWriter.preflight(
        outputTargets, options.generateUv2, protectedInputPaths);
    (void)outputPaths;

    if (options.generateUv2)
    {
        // UV2 generation is attempted per target: a failure only discards that
        // target with a warning; the rest of the batch still bakes.
        std::vector<ValidTarget> generatedTargets;
        generatedTargets.reserve(validTargets.size());
        for (const auto& target : validTargets)
        {
            try
            {
                generateUv2Atlas(target, options);
                generatedTargets.push_back(target);
            }
            catch (const std::exception& error)
            {
                result.skippedTargets.push_back({
                    target.assemblyTarget->outputIdentity,
                    std::string("UV2 generation failed: ") + error.what()
                });
                if (onWarning)
                {
                    onWarning(result.skippedTargets.back());
                }
            }
        }
        validTargets = std::move(generatedTargets);
        result.skippedCount = static_cast<uint32_t>(result.skippedTargets.size());
        if (validTargets.empty())
        {
            return result;
        }
    }

    loadTargetDrawables(engine, assembly.targets);

    auto context = std::make_shared<StandaloneBakerContext>(engine, assembly.scene);
    context->initialize({ options.lightmapSettings.resolution, options.lightmapSettings.resolution });
    context->updateScene(0.0f);

    std::vector<std::shared_ptr<StandaloneLightmapBaker>> bakers;
    bakers.reserve(validTargets.size());
    for (const auto& target : validTargets)
    {
        bakers.push_back(context->createBaker(target.assemblyTarget->node, options.lightmapSettings));
    }

    const uint32_t targetCount = static_cast<uint32_t>(bakers.size());
    for (uint32_t targetIndex = 0; targetIndex < targetCount; ++targetIndex)
    {
        const auto& target = validTargets[targetIndex];
        auto& baker = bakers[targetIndex];
        if (onProgress && !onProgress({ targetIndex, targetCount, 0, options.lightmapSettings.accumulationFrames }))
        {
            result.cancelled = true;
            break;
        }

        bool stopAfterTarget = false;
        while (baker->completedFrames() < options.lightmapSettings.accumulationFrames)
        {
            baker->update();
            if (onProgress && !onProgress({
                    targetIndex,
                    targetCount,
                    baker->completedFrames(),
                    options.lightmapSettings.accumulationFrames }))
            {
                stopAfterTarget = true;
                break;
            }
        }

        if (baker->completedFrames() == options.lightmapSettings.accumulationFrames)
        {
            const auto pixels = baker->readPixels();
            outputWriter.write(pixels, target.assemblyTarget->outputIdentity);
            ++result.bakedCount;
        }
        if (stopAfterTarget)
        {
            result.cancelled = true;
            break;
        }
    }

    return result;
}

}
