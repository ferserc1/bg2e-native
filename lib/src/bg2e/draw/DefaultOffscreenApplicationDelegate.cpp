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

#include <bg2e/draw/DefaultOffscreenApplicationDelegate.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/gpu/OffscreenSurface.hpp>

#include <stdexcept>
#include <utility>

namespace bg2e::draw {

RendererConfiguration DefaultOffscreenApplicationDelegate::withoutOverlays(RendererConfiguration configuration)
{
    configuration.overlays.clear();
    return configuration;
}

DefaultOffscreenApplicationDelegate::DefaultOffscreenApplicationDelegate(std::unique_ptr<Renderer> renderer,
    RendererConfiguration configuration)
    : DefaultRenderLoopDelegate(std::move(renderer), withoutOverlays(std::move(configuration)))
{
}

void DefaultOffscreenApplicationDelegate::init(Engine* engine)
{
    init(engine, engine ? engine->offscreenSurface() : nullptr);
}

void DefaultOffscreenApplicationDelegate::init(Engine* engine, gpu::OffscreenSurface* surface)
{
    if (!engine || !surface || engine->offscreenSurface() != surface)
    {
        throw std::invalid_argument("Offscreen delegate requires its Engine's offscreen surface");
    }
    _ready = _pending = _finalRecorded = _resolved = false;
    DefaultRenderLoopDelegate::init(engine);
}

void DefaultOffscreenApplicationDelegate::initScene()
{
    DefaultRenderLoopDelegate::initScene();
}

void DefaultOffscreenApplicationDelegate::resize(gpu::Size2D extent)
{
    DefaultRenderLoopDelegate::resize(extent);
}

void DefaultOffscreenApplicationDelegate::configure(const app::DrawOffscreenApplicationConfig& configuration)
{
    _job = configuration;
}

void DefaultOffscreenApplicationDelegate::extendSettingsCatalog(Renderer& /* renderer */, SettingsCatalog& /* catalog */)
{
}

void DefaultOffscreenApplicationDelegate::prepareFrame()
{
    DefaultRenderLoopDelegate::prepareFrame();
    auto* deferred = dynamic_cast<RendererDeferred*>(renderer());
    if (!renderer() || !renderer()->supportsLinearRecording() || !renderer()->configuration().overlays.empty())
    {
        throw std::invalid_argument("Scene averaging requires a linear-recording renderer without editor overlays");
    }
    if (!_ready)
    {
        _settings = std::make_unique<RenderSettingsPreferences>(_job.persistedDefaults);
        _settings->bind(*renderer(), scene(), [this](Renderer& active, SettingsCatalog& catalog)
        {
            extendSettingsCatalog(active, catalog);
        });
        _settingsResult = {};
        if (_job.graphicsDocument || _job.persistedDefaults)
        {
            _settingsResult = _settings->restore(_job.graphicsDocument.get(), _job.unknownSettings);
            if (!_settingsResult.valid())
            {
                std::string message = "Invalid headless graphics configuration";
                for (const auto& issue : _settingsResult.issues)
                {
                    message += "\n"+issue.path+": "+issue.message;
                }
                throw std::invalid_argument(message);
            }
        }
        if (_job.internalExtentOverride)
        {
            if (auto resolution = renderer()->configuration().resolution)
            {
                resolution->setEnabled(false);
            }
            setInternalExtent(*_job.internalExtentOverride);
        }
        else if (_job.scaleOverride)
        {
            auto resolution = renderer()->configuration().resolution;
            if (!resolution)
            {
                throw std::invalid_argument("Renderer does not expose a resolution scale policy");
            }
            resolution->setScale(*_job.scaleOverride);
            resolution->setEnabled(true);
        }
        DefaultRenderLoopDelegate::prepareFrame();
        if (deferred && !deferred->environmentError().empty())
        {
            throw std::invalid_argument(deferred->environmentError());
        }
        // G-buffer debug views are not an average of the reconstructed HDR scene.
        for (const auto& selection : renderer()->configuration().layers)
        {
            if (auto final = std::dynamic_pointer_cast<pipeline::FinalLayer>(selection.layer);
                final && final->settings.display.x != 0)
            {
                throw std::invalid_argument("Headless scene averaging requires the Final output view");
            }
        }
        renderer()->invalidateHistories();
        _accumulator.reset(renderer()->configuration().outputExtent, _job.frames);
        _generation = rendererGeneration();
        _sceneIdentity = scene().get();
        _settingsSnapshot = SettingsCodec::stringify(*SettingsCodec::serialize(_settings->catalog()));
        _sceneSnapshot.clear();
        _ready = true;
    }
    else if (rendererGeneration() != _generation || scene().get() != _sceneIdentity ||
        SettingsCodec::stringify(*SettingsCodec::serialize(_settings->catalog())) != _settingsSnapshot ||
        renderer()->configuration().outputExtent != _job.extent)
    {
        throw std::logic_error("Graphics configuration or scene changed during an accumulation job; start a new job");
    }
}

void DefaultOffscreenApplicationDelegate::update(float /* deltaSeconds */, uint64_t /* sceneFrameNumber */)
{
    // Renderer::update needs the recording FrameContext and runs in render().
}

void DefaultOffscreenApplicationDelegate::render(const FrameContext& frame)
{
    if (!_ready || _pending || _accumulator.complete())
    {
        throw std::logic_error("Headless accumulation is not ready for another sample");
    }
    DefaultRenderLoopDelegate::update(frame);
    auto& active = *renderer();
    if (SettingsCodec::stringify(*SettingsCodec::serialize(_settings->catalog())) != _settingsSnapshot)
    {
        throw std::logic_error("Graphics settings changed during a scene update");
    }
    auto signature = active.sceneFingerprint();
    if (_job.timeStepSeconds == 0 && signature.empty())
    {
        throw std::logic_error("Frozen accumulation requires a nonempty renderer scene identity");
    }
    if (_job.timeStepSeconds == 0 && !_sceneSnapshot.empty() && signature != _sceneSnapshot)
    {
        throw std::logic_error("Frozen scene or history generation changed during accumulation");
    }
    _sceneSnapshot = std::move(signature);
    _sample = active.recordLinear(frame);
    if (!_sample || _sample->device() != _engine->device())
    {
        throw std::logic_error("Renderer returned no linear scene sample");
    }
    _pendingSerial = frame.frameNumber;
    _pending = true;
}

void DefaultOffscreenApplicationDelegate::completeFrame(uint64_t sceneFrameNumber)
{
    if (!_pending || _pendingSerial != sceneFrameNumber)
    {
        throw std::logic_error("Headless scene completion does not match its recorded sample");
    }
    _accumulator.add(*_sample);
    _pending = false;
    if (_accumulator.complete())
    {
        _accumulator.resolveInto(*_sample);
    }
}

bool DefaultOffscreenApplicationDelegate::needsResolve() const
{
    return _accumulator.complete() && !_resolved;
}

void DefaultOffscreenApplicationDelegate::resolveFrame(const FrameContext& frame)
{
    if (!needsResolve() || _finalRecorded || frame.frameNumber != _pendingSerial)
    {
        throw std::logic_error("Headless final conversion requires the completed scene average");
    }
    renderer()->recordFinal(frame, *_sample);
    _finalRecorded = true; // Publication waits for confirmed GPU completion.
}

bool DefaultOffscreenApplicationDelegate::continueRendering() const
{
    return !_accumulator.complete();
}

void DefaultOffscreenApplicationDelegate::completeResolve()
{
    if (!_finalRecorded || _resolved)
    {
        throw std::logic_error("No pending final scene conversion");
    }
    _resolved = true;
}

void DefaultOffscreenApplicationDelegate::finish()
{
    if (!_accumulator.complete() || _pending || !_resolved)
    {
        throw std::logic_error("Headless job ended before all requested samples completed");
    }
}

uint32_t DefaultOffscreenApplicationDelegate::completedFrames() const
{
    return _accumulator.completedFrames();
}

bool DefaultOffscreenApplicationDelegate::hasResult() const
{
    return _resolved;
}

const SettingsResult& DefaultOffscreenApplicationDelegate::settingsResult() const
{
    return _settingsResult;
}

void DefaultOffscreenApplicationDelegate::setRenderer(std::unique_ptr<Renderer> renderer,
    RendererConfiguration configuration)
{
    DefaultRenderLoopDelegate::setRenderer(std::move(renderer), withoutOverlays(std::move(configuration)));
}

void DefaultOffscreenApplicationDelegate::cleanup()
{
    if (_settings)
    {
        _settings->clear();
        _settings.reset();
    }
    _sample.reset();
    _ready = _pending = false;
    DefaultRenderLoopDelegate::cleanup();
}

}
