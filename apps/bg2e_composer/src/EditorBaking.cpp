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

#include "AppDelegate.hpp"

#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/Renderer.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/Texture.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/db/texture_archive.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <set>
#include <stdexcept>

namespace bg2e_composer {

void AppDelegate::render(const bg2e::draw::FrameContext& frame)
{
    if (_bakeContext && _bakeRendererGeneration != rendererGeneration())
    {
        stopLightmapBake(true);
        _status = "Bake cancelled because the renderer binding changed";
    }
    DefaultRenderLoopDelegate::render(frame);
    if (!_baker || !_bakeContext || !_stage || !_stage->document().root ||
        _bakeDocumentGeneration != _stage->document().generation || _bakeSubmission)
    {
        return;
    }

    const auto progress = _baker->progress();
    if (progress.state != bg2e::bake::BakeState::Ready && progress.state != bg2e::bake::BakeState::Running)
    {
        return;
    }
    try
    {
        _bakeContext->prepareFrame(frame);
        _bakeSubmission = BakeSubmission{frame.frameSlot, frame.frameNumber};
        _baker->record({frame.commandBuffer, frame.frameSlot, frame.frameNumber, _bakeContext->sourceGeneration()});
    }
    catch (const std::exception& error)
    {
        _status = std::string("Bake recording failed: ") + error.what();
        if (_bakeContext && !_bakeSubmission)
        {
            stopLightmapBake(true);
        }
    }
}

void AppDelegate::retireBakeFrame()
{
    if (!_bakeSubmission || !_bakeContext)
    {
        return;
    }
    const auto submitted = *_bakeSubmission;
    _bakeSubmission.reset();
    try
    {
        _engine->device()->waitIdle();
        _bakeContext->completeFrame(submitted.slot, submitted.serial);
    }
    catch (const std::exception& error)
    {
        _status = std::string("Bake completion failed: ") + error.what();
    }

    try
    {
        if (_baker)
        {
            const auto progress = _baker->progress();
            if (progress.state == bg2e::bake::BakeState::Complete)
            {
                auto& target = _bakeTargets.at(_bakeTargetIndex);
                target.result = _baker->result();
                target.pixels = _baker->readPixels();
                _baker->cleanup();
                _baker.reset();
                ++_bakeTargetIndex;
                if (_bakeTargetIndex < _bakeTargets.size())
                {
                    bg2e::bake::LightmapBakerConfiguration configuration;
                    configuration.target = _bakeTargets[_bakeTargetIndex].node.lock();
                    configuration.settings = std::make_shared<bg2e::bake::LightmapSettings>(_bakeSettings);
                    _baker = _bakeContext->createBaker(std::move(configuration));
                    _bakeTarget = _bakeTargets[_bakeTargetIndex].node;
                    return;
                }
                setContinuousRendering(_wasContinuousRendering);
                requestSceneFrame();
                if (!_bakeApplyQueued)
                {
                    _bakeApplyQueued = true;
                    applyLightmapBake();
                }
            }
            else if (progress.state == bg2e::bake::BakeState::Invalidated ||
                progress.state == bg2e::bake::BakeState::Failed ||
                progress.state == bg2e::bake::BakeState::Cancelled)
            {
                setContinuousRendering(_wasContinuousRendering);
                requestSceneFrame();
                if (!progress.status.empty())
                {
                    _status = progress.status;
                }
            }
        }
    }
    catch (const std::exception& error)
    {
        stopLightmapBake(true);
        _status = std::string("Bake batch completion failed: ") + error.what();
    }
}

void AppDelegate::stopLightmapBake(bool cancelled)
{
    const bool hadContext = static_cast<bool>(_bakeContext);
    if (cancelled && _baker)
    {
        _baker->cancel();
    }
    retireBakeFrame();
    if (_baker)
    {
        try
        {
            if (cancelled)
            {
                _baker->cancel();
            }
            _baker->cleanup();
        }
        catch (const std::exception& error)
        {
            _status = std::string("Bake cleanup failed: ") + error.what();
        }
        _baker.reset();
    }
    if (_bakeContext)
    {
        try
        {
            _bakeContext->cleanup();
        }
        catch (const std::exception& error)
        {
            _status = std::string("Bake context cleanup failed: ") + error.what();
        }
        _bakeContext.reset();
    }
    if (hadContext)
    {
        setContinuousRendering(_wasContinuousRendering);
    }
    _bakeTarget.reset();
    _bakeTargets.clear();
    _bakeTargetIndex = 0;
    _bakeDocumentGeneration = 0;
}

void AppDelegate::invalidateLightmapSources()
{
    if (_baker || _bakeContext)
    {
        stopLightmapBake(true);
        _status = "Bake cancelled because the document sources changed";
    }
}

std::shared_ptr<bg2e::app::SafeUpdateToken> AppDelegate::regenerateUv2(std::weak_ptr<bg2e::scene::Node> target,
    const bg2e::geo::Uv2AtlasOptions& options,
    bg2e::app::Uv2SafeReload::CompletionCallback completion)
{
    if (!_stage || !document() || !document()->root)
    {
        if (completion)
        {
            completion({false, "Open a document before generating UV2"});
        }
        return {};
    }
    const auto generation = document()->generation;
    const auto weak = weak_from_this();
    auto delegate = std::static_pointer_cast<bg2e::draw::DefaultRenderLoopDelegate>(shared_from_this());
    return bg2e::app::Uv2SafeReload::regenerateDraw(delegate, std::move(target), options,
        [weak, generation, completion = std::move(completion)](const bg2e::app::Uv2RegenerationResult& result)
        {
            auto owner = weak.lock();
            if (!owner || !result.success)
            {
                if (completion)
                {
                    completion(result);
                }
                return;
            }
            owner->enqueueRendererUpdate(owner->rendererGeneration(),
                [weak, generation, result, completion](bg2e::draw::Renderer& renderer)
                {
                    auto active = weak.lock();
                    if (!active || !active->document() || active->document()->generation != generation)
                    {
                        if (completion)
                        {
                            completion({false, "Document changed before UV2 preview refresh"});
                        }
                        return;
                    }
                    active->_stage->prepareTools();
                    active->_selection->refreshSelection();
                    active->notifyEdited(renderer);
                    active->_status = "UV2 atlas generated";
                    if (completion)
                    {
                        completion(result);
                    }
                });
        },
        [weak, generation]
        {
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                throw std::logic_error("Document changed before UV2 regeneration");
            }
            owner->invalidateLightmapSources();
        });
}

void AppDelegate::startLightmapBake(std::weak_ptr<bg2e::scene::Node> target,
    const bg2e::bake::LightmapSettings& settings)
{
    startLightmapTargets({std::move(target)}, settings);
}

void AppDelegate::startLightmapBatch(bool wholeScene, const bg2e::bake::LightmapSettings& settings)
{
    std::vector<std::weak_ptr<bg2e::scene::Node>> targets;
    std::set<bg2e::scene::Node*> seen;
    std::function<void(const std::shared_ptr<bg2e::scene::Node>&)> collect;
    collect = [&](const auto& node)
    {
        if (!node || !seen.insert(node.get()).second)
        {
            return;
        }
        if (auto component = node->drawable(); component && component->drawable())
        {
            targets.push_back(node);
        }
        for (const auto& child : node->children())
        {
            collect(child);
        }
    };
    if (wholeScene && document())
    {
        collect(document()->root);
    }
    else if (_selection)
    {
        for (const auto& node : _selection->selectedNodes())
        {
            collect(node.lock());
        }
    }
    startLightmapTargets(std::move(targets), settings);
}

void AppDelegate::startLightmapTargets(std::vector<std::weak_ptr<bg2e::scene::Node>> targets,
    const bg2e::bake::LightmapSettings& settings)
{
    if (!_stage || !document() || !document()->root || targets.empty())
    {
        _status = "Select at least one document drawable before starting a lightmap bake";
        return;
    }
    const auto generation = document()->generation;
    const auto weak = weak_from_this();
    enqueueRendererUpdate(rendererGeneration(),
        [weak, targets = std::move(targets), generation, settings](bg2e::draw::Renderer& renderer)
        {
            auto owner = weak.lock();
            auto node = targets.front().lock();
            if (!owner || !node || !owner->_stage || !owner->document() ||
                owner->document()->generation != generation || !owner->_stage->contains(node.get()))
            {
                return;
            }
            const bool wasContinuous = owner->_bakeContext
                ? owner->_wasContinuousRendering : owner->continuousRendering();
            owner->stopLightmapBake(true);
            owner->_completedLightmap.reset();
            owner->_completedLightmaps.clear();
            owner->_bakeApplyQueued = false;
            owner->_wasContinuousRendering = wasContinuous;
            try
            {
                std::string warnings;
                for (const auto& weakTarget : targets)
                {
                    auto target = weakTarget.lock();
                    auto component = target ? target->drawable() : nullptr;
                    auto drawable = component ? component->drawable() : nullptr;
                    if (!target || !owner->_stage->contains(target.get()) || !drawable || !drawable->isLoaded())
                    {
                        throw std::logic_error("A batch target is no longer loaded in this document");
                    }
                    const auto validation = bg2e::geo::UvAtlasValidator::validate(*drawable->mesh(), 1,
                        bg2e::geo::UvAtlasOverlapPolicy::Warn);
                    if (!validation.valid)
                    {
                        throw std::invalid_argument(target->name() + ": " + validation.message);
                    }
                    if (!validation.warning.empty())
                    {
                        warnings += " Warning: " + target->name() + ": " + validation.warning;
                    }
                    owner->_bakeTargets.push_back({target, drawable, {}, {}});
                }
                owner->_bakeSettings = settings;
                bg2e::bake::BakerContextConfiguration contextConfiguration;
                contextConfiguration.device = owner->_engine->device();
                contextConfiguration.backend = owner->_engine->backendType();
                contextConfiguration.frameSlots = owner->_engine->frameSlotCount();
                contextConfiguration.sceneRoot = owner->document()->root;
                if (auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(&renderer);
                    deferred && deferred->environmentGenerator())
                {
                    contextConfiguration.environment = deferred->environmentGenerator()->resources();
                }
                owner->_bakeContext = std::make_shared<bg2e::bake::IntegratedBakerContext>(
                    *owner->_engine, std::move(contextConfiguration));
                bg2e::bake::LightmapBakerConfiguration bakerConfiguration;
                bakerConfiguration.target = node;
                bakerConfiguration.settings = std::make_shared<bg2e::bake::LightmapSettings>(settings);
                owner->_baker = owner->_bakeContext->createBaker(std::move(bakerConfiguration));
                owner->_bakeTarget = node;
                owner->_bakeDocumentGeneration = generation;
                owner->_bakeRendererGeneration = owner->rendererGeneration();
                owner->setContinuousRendering(true);
                renderer.invalidateHistories();
                owner->_status = "Lightmap bake started" + warnings;
            }
            catch (const std::exception& error)
            {
                owner->stopLightmapBake(true);
                owner->_status = std::string("Could not start lightmap bake: ") + error.what();
            }
        });
}

void AppDelegate::cancelLightmapBake()
{
    const auto weak = weak_from_this();
    enqueueRendererUpdate(rendererGeneration(), [weak](bg2e::draw::Renderer&)
    {
        if (auto owner = weak.lock())
        {
            owner->stopLightmapBake(true);
            owner->_status = "Lightmap bake cancelled";
        }
    });
}

bool AppDelegate::lightmapBakeActive() const
{
    if (!_baker)
    {
        return false;
    }
    const auto state = _baker->progress().state;
    return state == bg2e::bake::BakeState::Ready || state == bg2e::bake::BakeState::Running;
}

bg2e::bake::BakeProgress AppDelegate::lightmapBakeProgress() const
{
    if (_baker)
    {
        auto progress = _baker->progress();
        progress.status = "Target " + std::to_string(_bakeTargetIndex + 1) + " / " +
            std::to_string(_bakeTargets.size()) + ": " + progress.status;
        return progress;
    }
    if (document() && document()->generation == _completedLightmapDocumentGeneration && _completedLightmap)
    {
        return {bg2e::bake::BakeState::Complete, 0, 0, 0, 0, _status};
    }
    return {bg2e::bake::BakeState::Ready, 0, 0, 0, 0, _status};
}

std::optional<bg2e::bake::LightmapResult> AppDelegate::lightmapResult(
    const std::shared_ptr<bg2e::scene::Node>& target) const
{
    if (target)
    {
        if (!document() || document()->generation != _completedLightmapDocumentGeneration)
        {
            return std::nullopt;
        }
        for (const auto& [node, result] : _completedLightmaps)
        {
            if (node.lock() == target)
            {
                return result;
            }
        }
        return std::nullopt;
    }
    if (!_baker)
    {
        return document() && document()->generation == _completedLightmapDocumentGeneration
            ? _completedLightmap : std::nullopt;
    }
    return _baker->result();
}

void AppDelegate::applyLightmapBake()
{
    if (_bakeTargets.empty() || _bakeTargetIndex != _bakeTargets.size() || !_stage || !_stage->document().root)
    {
        return;
    }
    const auto generation = _bakeDocumentGeneration;
    const auto weak = weak_from_this();
    const auto context = std::weak_ptr<bg2e::bake::IntegratedBakerContext>(_bakeContext);
    enqueueRendererUpdate(rendererGeneration(), [weak, generation, context](bg2e::draw::Renderer& renderer)
    {
        auto owner = weak.lock();
        if (!owner || !owner->_bakeContext || owner->_bakeContext != context.lock() || !owner->_stage || !owner->document() ||
            owner->document()->generation != generation || owner->_bakeTargets.empty() ||
            owner->_bakeTargetIndex != owner->_bakeTargets.size())
        {
            if (owner)
            {
                owner->_status = "Completed lightmap is stale; generate it again before applying";
            }
            return;
        }
        std::vector<std::filesystem::path> stagedDirectories;
        bool materialMutated = false;
        try
        {
            // Prepare every filesystem source and GPU texture before mutating
            // any receiver. All results were captured against the same scene.
            std::vector<std::shared_ptr<bg2e::draw::Texture>> textures;
            for (const auto& job : owner->_bakeTargets)
            {
                auto target = job.node.lock();
                auto component = target ? target->drawable() : nullptr;
                if (!target || !owner->_stage->contains(target.get()) || !component ||
                    component->drawable() != job.drawable || !job.result || !job.pixels)
                {
                    throw std::logic_error("Completed batch target or drawable is stale");
                }
                const auto& result = job.result;
                // Keep an authored filesystem source alive after the baker is released.
                // Saving the document then copies this source with its other textures.
                static std::atomic_uint64_t counter{0};
                std::filesystem::path directory;
                do
                {
                    directory = std::filesystem::temp_directory_path() /
                        ("bg2e_draw_bg2e_composer_lightmap_" + std::to_string(
                            std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
                            std::to_string(++counter));
                }
                while (!std::filesystem::create_directory(directory));
                stagedDirectories.push_back(directory);
                const auto& pixels = *job.pixels;
                if (pixels.format == bg2e::bake::LightmapPixelFormat::RGB32F)
                {
                    for (const auto value : std::get<std::vector<float>>(pixels.rgb))
                    {
                        if (!std::isfinite(value) || value < 0.0f || value > 65504.0f)
                        {
                            throw std::range_error("HDR lightmap exceeds the finite half-float display range");
                        }
                    }
                }
                const auto path = directory / (pixels.format == bg2e::bake::LightmapPixelFormat::RGB8
                    ? "lightmap.png" : "lightmap.bg2tex");
                if (pixels.format == bg2e::bake::LightmapPixelFormat::RGB8)
                {
                    const auto& rgb = std::get<std::vector<uint8_t>>(pixels.rgb);
                    bg2e::db::saveImage(path, rgb.data(), pixels.width, pixels.height, 3);
                }
                else
                {
                    // Preserve HDR multipliers instead of clamping them into PNG.
                    bg2e::db::writeTextureArchive(path, *result->image);
                }
                auto settings = std::make_shared<bg2e::base::Texture>(path);
                settings->setColorType(bg2e::base::Color::TypeLinear);
                settings->setUseMipmaps(false);
                settings->setAddressMode(bg2e::base::Texture::AddressModeClampToEdge);
                const auto filtering = owner->_engine->device()->formatCapabilities(result->image->pixelFormat()).linearFiltering
                    ? bg2e::base::Texture::FilterLinear : bg2e::base::Texture::FilterNearest;
                settings->setMinFilter(filtering);
                settings->setMagFilter(filtering);
                auto texture = std::make_shared<bg2e::draw::Texture>(owner->_engine);
                texture->load(settings);
                textures.push_back(std::move(texture));
            }
            for (size_t index = 0; index < owner->_bakeTargets.size(); ++index)
            {
                const auto& job = owner->_bakeTargets[index];
                const auto& result = job.result;
                const auto& texture = textures[index];
                const auto& drawable = job.drawable;
                for (uint32_t submesh = 0; submesh < drawable->submeshesCount(); ++submesh)
                {
                    auto material = drawable->drawMaterial(submesh);
                    materialMutated = true;
                    material->setTexture(bg2e::draw::TextureSlot::AmbientOcclusion, texture);
                    auto& attributes = material->materialAttributes();
                    attributes.setAoUVSet(result->uvSet);
                    attributes.setAoScale(glm::vec2{1.0f, 1.0f});
                    attributes.setIndirectTextureMode(result->mode == bg2e::bake::LightmapMode::RTAO
                        ? bg2e::base::MaterialAttributes::IndirectTextureMode::BakedAmbientOcclusion
                        : bg2e::base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination);
                    drawable->materialDefinition(submesh) = bg2e::draw::cloneMaterialAttributes(attributes);
                }
                // Retain applied images independently of the context released by notifyEdited.
                owner->_completedLightmap = result;
                owner->_completedLightmap->image = texture->imageResource();
                owner->_completedLightmapDocumentGeneration = generation;
                owner->_completedLightmaps.emplace_back(job.node, *owner->_completedLightmap);
            }
            const auto count = owner->_bakeTargets.size();
            owner->_stage->prepareTools();
            owner->notifyEdited(renderer);
            owner->_status = "Lightmaps applied to " + std::to_string(count) + " targets; save the scene to publish resources";
        }
        catch (const std::exception& error)
        {
            if (!materialMutated)
            {
                for (const auto& directory : stagedDirectories)
                {
                    std::error_code ignored;
                    std::filesystem::remove_all(directory, ignored);
                }
            }
            owner->_stage->markModified();
            owner->invalidateLightmapSources();
            renderer.invalidateHistories();
            owner->requestSceneFrame();
            owner->_status = std::string("Lightmap apply failed; document may contain partial material updates: ") + error.what();
        }
    });
}

}
