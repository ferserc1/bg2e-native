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

#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/draw/DefaultOffscreenApplicationDelegate.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/Factory.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/OffscreenSurface.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/geo/plane.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace bg2e;

void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

class Probe final : public app::DrawOffscreenApplicationDelegate {
  public:
    explicit Probe(gpu::BackendType selected, uint64_t requested = 2) : backend(selected), requestedFrames(requested)
    {
    }

    void initConfig(int, char**, app::DrawOffscreenApplicationConfig& config) override
    {
        config.engine.backend = backend;
        config.engine.colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
        config.extent = {7, 3};
        config.createDepthImage = false;
        config.timeStepSeconds = 0.125f;
    }

    void init(draw::Engine* value, gpu::OffscreenSurface* target) override
    {
        engine = value;
        surface = target;
        require(!engine->surface() && engine->renderSurface() == target, "Expected generic offscreen surface");
        require(!engine->instance()->window() && engine->instance()->presentationMode() == gpu::PresentationMode::Offscreen,
            "Headless engine must not initialize a windowed instance");
        require(engine->frameSlotCount() == 1 && !target->depthImage(), "Unexpected headless slots/depth");
        if (failInit)
        {
            throw std::runtime_error("expected init failure");
        }
    }

    void update(float deltaSeconds, uint64_t index) override
    {
        require(index == completed && deltaSeconds == (index ? 0.125f : 0.0f), "Fixed time/scene index mismatch");
    }

    void render(const draw::FrameContext& frame) override
    {
        require(frame.frameSlot == 0 && frame.frameNumber == completed && frame.sceneFrameNumber == completed,
            "Frame chronology mismatch");
        require(frame.sceneDeltaSeconds == (completed ? 0.125f : 0.0f), "Frame time mismatch");
        frame.commandBuffer.beginRendering(&frame.colorTarget);
        frame.commandBuffer.clearColor(0, completed == 0 ? gpu::Color{1, 0, 0, 1} : gpu::Color{0, 1, 0, 1});
        frame.commandBuffer.endRendering();
        ++recorded;
        if (failRecord)
        {
            throw std::runtime_error("expected record failure");
        }
    }

    void didRenderFrame(uint64_t index, double elapsedMs) override
    {
        require(index == completed && elapsedMs >= 0 && std::isfinite(elapsedMs), "Completion chronology mismatch");
        require(surface->frameCounter() == completed + 1, "Surface counter is not completed scene chronology");
        std::vector<uint8_t> pixels;
        surface->colorImage(0)->readPixels(pixels, {}, gpu::ImageLayout::TransferSrc);
        require(pixels.size() == 7 * 3 * 4, "Tight readback dimensions mismatch");
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            require(pixels[i] == (completed == 0 ? 255 : 0) && pixels[i + 1] == (completed == 0 ? 0 : 255) &&
                pixels[i + 2] == 0 && pixels[i + 3] == 255, "Completed clear output mismatch");
        }
        ++completed;
        if (failCompletion)
        {
            throw std::runtime_error("expected completion failure");
        }
    }

    bool continueRendering() const override
    {
        return completed < requestedFrames;
    }

    void cleanup() override
    {
        ++cleaned;
        require(engine && engine->device()->isValid(), "Delegate cleanup must precede Engine destruction");
        engine = nullptr;
        surface = nullptr;
        if (failCleanup)
        {
            throw std::runtime_error("expected cleanup failure");
        }
    }

    gpu::BackendType backend;
    uint64_t requestedFrames, recorded = 0, completed = 0;
    unsigned cleaned = 0;
    bool failInit = false, failRecord = false, failCompletion = false, failCleanup = false;

  private:
    draw::Engine* engine = nullptr;
    gpu::OffscreenSurface* surface = nullptr;
};

void execute(const std::shared_ptr<Probe>& probe, const std::string& expected = {})
{
    std::string failure;
    {
        app::OffscreenApplication application;
        try
        {
            application.init(0, nullptr, "bg2e.offscreen.probe", probe);
            require(application.run() == 0, "Unexpected exit code");
        }
        catch (const std::exception& error)
        {
            failure = error.what();
        }
    }
    require(failure == expected, "Unexpected failure or exception masking");
    require(probe->cleaned == 1, "Delegate must be cleaned exactly once");
}

void sharedInstance(gpu::BackendType selected)
{
    auto backend = gpu::Factory::acquireBackend(selected);
    draw::EngineConfig config;
    config.backend = selected;
    config.depthFormat = gpu::PixelFormat::Undefined;
    draw::Engine first, second;
    first.init(gpu::Size2D{4, 3}, *backend, config);
    bool rejected = false;
    try
    {
        second.init(gpu::Size2D{4, 3}, *backend, config);
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected && first.device()->isValid() && first.renderSurface()->isValid(),
        "Second context must not overwrite or clean the active shared Instance");
    first.cleanup();
}

struct AverageCounts {
    unsigned linear = 0, finalPasses = 0;
};

class ControlledRenderer final : public draw::RendererDeferred {
  public:
    ControlledRenderer(std::shared_ptr<AverageCounts> counts, bool varying)
        : _counts(std::move(counts)), _varying(varying)
    {
    }

    std::shared_ptr<gpu::Image> recordLinear(const draw::FrameContext& frame) override
    {
        auto image = RendererDeferred::recordLinear(frame);
        const float value = _varying ? (frame.sceneFrameNumber == 0 ? 0.25f : 0.75f) : 0.5f;
        draw::pipeline::RenderTarget::clear(frame.commandBuffer, *image, {value,value,value,1});
        ++_counts->linear;
        return image;
    }

    void recordFinal(const draw::FrameContext& frame, gpu::Image& linear) override
    {
        ++_counts->finalPasses;
        RendererDeferred::recordFinal(frame, linear);
    }

  private:
    std::shared_ptr<AverageCounts> _counts;
    bool _varying;
};

class AverageProbe final : public draw::DefaultOffscreenApplicationDelegate {
  public:
    AverageProbe(gpu::BackendType backend, std::shared_ptr<AverageCounts> counts, bool varying)
        : DefaultOffscreenApplicationDelegate(std::make_unique<ControlledRenderer>(counts,varying)),
          _backend(backend), _counts(std::move(counts)), _varying(varying)
    {
    }

    void initConfig(int, char**, app::DrawOffscreenApplicationConfig& config) override
    {
        config.engine.backend = _backend;
        config.extent = {8,6};
        config.frames = _varying ? 2 : 1;
        config.scaleOverride = 0.5f;
    }

    void didRenderFrame(uint64_t index, double) override
    {
        const unsigned expected = _varying ? 2 : 1;
        require(completedFrames() == index+1, "Only completed scene samples must count");
        require(renderer()->configuration().internalExtent == gpu::Size2D{4,3}, "Scale must preserve output extent");
        if (index+1 == expected)
        {
            require(hasResult() && _counts->finalPasses == 1, "Final conversion must complete once");
            _engine->offscreenSurface()->colorImage(0)->readPixels(output, {}, gpu::ImageLayout::TransferSrc);
        }
        else
        {
            require(!hasResult() && _counts->finalPasses == 0, "Partial averages must not reach final conversion");
        }
    }

    std::vector<uint8_t> output;

  private:
    gpu::BackendType _backend;
    std::shared_ptr<AverageCounts> _counts;
    bool _varying;
};

void checkAverage(gpu::BackendType backend)
{
    auto singleCounts = std::make_shared<AverageCounts>();
    auto single = std::make_shared<AverageProbe>(backend,singleCounts,false);
    {
        app::OffscreenApplication application;
        application.init(0,nullptr,"bg2e.offscreen.single",single);
        application.run();
    }
    auto varyingCounts = std::make_shared<AverageCounts>();
    auto varying = std::make_shared<AverageProbe>(backend,varyingCounts,true);
    {
        app::OffscreenApplication application;
        application.init(0,nullptr,"bg2e.offscreen.average",varying);
        application.run();
    }
    require(singleCounts->linear == 1 && varyingCounts->linear == 2, "Wrong requested scene sample count");
    require(!single->output.empty() && single->output == varying->output,
        "Average of linear 0.25/0.75 must match single linear 0.5 after final conversion");
}

class FrozenMotionProbe final : public draw::DefaultOffscreenApplicationDelegate {
  public:
    explicit FrozenMotionProbe(gpu::BackendType backend)
        : DefaultOffscreenApplicationDelegate(std::make_unique<draw::RendererDeferred>()), _backend(backend)
    {
    }

    void initConfig(int, char**, app::DrawOffscreenApplicationConfig& config) override
    {
        config.engine.backend = _backend;
        config.extent = {8,6};
        config.frames = 2;
        config.timeStepSeconds = 0;
    }

    void didRenderFrame(uint64_t index, double) override
    {
        require(index == 0 && completedFrames() == 1, "Frozen motion job must complete only its initial sample");
        auto* deferred = dynamic_cast<draw::RendererDeferred*>(renderer());
        require(deferred != nullptr, "Expected deferred fingerprint implementation");
        _generation = deferred->historyGeneration();
        auto moved = glm::mat4(1);
        moved[3].x = 0.1f;
        _moving->setTransform(moved); // Completed stopped boundary, no scene replacement.
    }

    void render(const draw::FrameContext& frame) override
    {
        try
        {
            DefaultOffscreenApplicationDelegate::render(frame);
        }
        catch (const std::logic_error& error)
        {
            if (frame.sceneFrameNumber == 1 &&
                std::string(error.what()) == "Frozen scene or history generation changed during accumulation")
            {
                auto* deferred = dynamic_cast<draw::RendererDeferred*>(renderer());
                require(deferred && deferred->historyGeneration() == _generation,
                    "Frozen motion rejection must rely on full identity with unchanged history generation");
                rejectedMotion = true;
            }
            throw;
        }
    }

    void cleanup() override
    {
        _moving.reset();
        DefaultOffscreenApplicationDelegate::cleanup();
    }

    bool rejectedMotion = false;

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("frozen motion probe");
        _moving = std::make_shared<draw::Drawable>();
        _moving->setRayTracingEnabled(false);
        _moving->setMesh(geo::createPlane(1,1));
        _moving->load(_engine);
        root->addComponent(new scene::DrawableComponent(_moving));
        return root;
    }

  private:
    gpu::BackendType _backend;
    std::shared_ptr<draw::Drawable> _moving;
    uint64_t _generation = 0;
};

void checkFrozenMotion(gpu::BackendType backend)
{
    auto probe = std::make_shared<FrozenMotionProbe>(backend);
    std::string failure;
    {
        app::OffscreenApplication application;
        application.init(0,nullptr,"bg2e.offscreen.frozen_motion",probe);
        try
        {
            application.run();
        }
        catch (const std::logic_error& error)
        {
            failure = error.what();
        }
    }
    require(probe->rejectedMotion && failure == "Frozen scene or history generation changed during accumulation",
        "Real offscreen delegate must reject a transform edit in a frozen job");
}

class DeferredProbe final : public draw::DefaultOffscreenApplicationDelegate {
  public:
    explicit DeferredProbe(gpu::BackendType backend)
        : DefaultOffscreenApplicationDelegate(std::make_unique<draw::RendererDeferred>()), _backend(backend)
    {
    }

    void initConfig(int, char**, app::DrawOffscreenApplicationConfig& config) override
    {
        config.engine.backend = _backend;
        config.extent = {64, 48};
        config.createDepthImage = false;
    }

    void didRenderFrame(uint64_t index, double) override
    {
        require(index == 0, "Default renderer delegate must submit exactly one frame");
        ++completed;
    }

    unsigned completed = 0;

  private:
    gpu::BackendType _backend;
};
}

int main(int argc, char** argv)
{
    try
    {
        auto selected = bg2e::draw::EngineConfig{}.backend;
        if (argc > 1)
        {
            const std::string argument = argv[1];
            require(argument == "vulkan" || argument == "metal", "Expected vulkan or metal");
            selected = argument == "metal" ? bg2e::gpu::BackendType::Metal : bg2e::gpu::BackendType::Vulkan;
        }
        auto normal = std::make_shared<Probe>(selected);
        execute(normal);
        require(normal->completed == 2, "Both requested scene frames must complete");
        auto zero = std::make_shared<Probe>(selected, 0);
        execute(zero);
        require(zero->recorded == 0 && zero->completed == 0, "False continuation must submit no frame");
        auto partial = std::make_shared<Probe>(selected);
        partial->failInit = true;
        execute(partial, "expected init failure");
        auto record = std::make_shared<Probe>(selected);
        record->failRecord = record->failCleanup = true;
        execute(record, "expected record failure");
        require(record->completed == 0, "Failed recording must not notify completion");
        auto completion = std::make_shared<Probe>(selected);
        completion->failCompletion = true;
        execute(completion, "expected completion failure");
        auto cleanup = std::make_shared<Probe>(selected);
        cleanup->failCleanup = true;
        execute(cleanup, "expected cleanup failure");
        auto abandoned = std::make_shared<Probe>(selected);
        {
            bg2e::app::OffscreenApplication application;
            application.init(0, nullptr, "bg2e.offscreen.abandoned", abandoned);
        }
        require(abandoned->cleaned == 1 && abandoned->recorded == 0, "Destruction before run must release the context");
        sharedInstance(selected);
        if (argc > 2 && std::string(argv[2]) == "deferred")
        {
            auto deferred = std::make_shared<DeferredProbe>(selected);
            bg2e::app::OffscreenApplication application;
            application.init(0, nullptr, "bg2e.offscreen.deferred", deferred);
            require(deferred->renderer() && deferred->renderer()->configuration().overlays.empty(),
                "Headless renderer must initialize with no overlays");
            application.run();
            require(deferred->completed == 1, "Empty-scene deferred frame did not complete");
            checkAverage(selected);
            checkFrozenMotion(selected);
        }
        std::cout << "Offscreen lifecycle probe passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
