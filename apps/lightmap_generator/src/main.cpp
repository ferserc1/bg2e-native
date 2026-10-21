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

#include "CommandLine.hpp"
#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/bake/StandaloneBakeSceneAssembler.hpp>
#include <bg2e/bake/StandaloneBakeBatch.hpp>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
volatile std::sig_atomic_t interrupted = 0;

void handleSignal(int signal)
{
    interrupted = signal;
}

class SignalScope final {
  public:
    SignalScope()
    {
        _interrupt = std::signal(SIGINT, handleSignal);
        if (_interrupt == SIG_ERR)
        {
            throw std::runtime_error("Could not install SIGINT handler");
        }
        _terminate = std::signal(SIGTERM, handleSignal);
        if (_terminate == SIG_ERR)
        {
            std::signal(SIGINT, _interrupt);
            throw std::runtime_error("Could not install SIGTERM handler");
        }
    }
    ~SignalScope()
    {
        std::signal(SIGINT, _interrupt);
        std::signal(SIGTERM, _terminate);
    }
    SignalScope(const SignalScope&) = delete;
    SignalScope& operator=(const SignalScope&) = delete;

  private:
    using Handler = void (*)(int);
    Handler _interrupt = SIG_DFL, _terminate = SIG_DFL;
};

class BakeApplication final : public bg2e::app::DrawOffscreenApplicationDelegate {
  public:
    explicit BakeApplication(draw_lightmap_generator::Options options) : _options(std::move(options))
    {
    }

    void initConfig(int, char**, bg2e::app::DrawOffscreenApplicationConfig& config) override
    {
        config.engine = _options.engine;
        config.extent = {1, 1}; // Context surface only; all UV surfaces belong to the baker.
        config.createDepthImage = false;
        config.frames = 1;
        // Configuration was validated against live bake settings before GPU init.
        // No preferences or camera-renderer graphics settings are consumed here.
    }
    void init(bg2e::draw::Engine* engine, bg2e::gpu::OffscreenSurface*) override
    {
        _engine = engine;
    }

    bool continueRendering() const override
    {
        return false; // Standalone baker owns submissions; no camera frames.
    }
    void render(const bg2e::draw::FrameContext&) override
    {
        throw std::logic_error("A standalone UV batch must not record a camera frame");
    }
    void finish() override
    {
        if (interrupted)
        {
            _result.cancelled = true;
            return;
        }
        bg2e::bake::StandaloneBakeSceneAssembler assembler(*_engine);
        std::optional<bg2e::geo::Uv2AtlasOptions> uv2;
        if (_options.generateUv2)
        {
            uv2 = bg2e::geo::Uv2AtlasOptions{_options.batch.settings.resolution(), _options.uv2Padding};
        }
        // Private candidates and all native target owners die inside finish(),
        // before OffscreenApplication drains/cleans its borrowed Engine.
        auto assembly = _options.command == draw_lightmap_generator::Command::Model ?
            assembler.assembleModel(_options.contextPath, _options.inputPath, uv2, false) :
            assembler.assemblePrefab(_options.contextPath, _options.inputPath, uv2, false);
        auto options = _options.batch;
        options.current = []
        {
            return interrupted == 0;
        };
        _result = bg2e::bake::StandaloneBakeBatch::run(*_engine, assembly, _options.outputDirectory, options,
            [](const bg2e::bake::StandaloneBakeBatch::Progress& progress)
            {
                if (progress.completedSamples == 0)
                {
                    std::cout << "Baking target " << (progress.targetIndex + 1) << "/" << progress.targetCount
                        << " (" << progress.requestedSamples << " completed UV samples requested)\n";
                }
                return interrupted == 0;
            });
    }
    void cleanup() override
    {
        _engine = nullptr;
    }

    const bg2e::bake::StandaloneBakeBatch::Result& result() const
    {
        return _result;
    }

  private:
    draw_lightmap_generator::Options _options;
    bg2e::draw::Engine* _engine = nullptr;
    bg2e::bake::StandaloneBakeBatch::Result _result;
};
}

int main(int argc, char** argv)
{
    if (draw_lightmap_generator::helpRequested(argc, argv))
    {
        draw_lightmap_generator::printUsage(std::cout);
        return 0;
    }
    draw_lightmap_generator::Options options;
    try
    {
        options = draw_lightmap_generator::parseOptions(argc, argv);
        draw_lightmap_generator::validateOptions(options);
        for (const auto& issue : options.diagnostics)
        {
            std::cerr << "Warning: " << issue.path << ": " << issue.message << '\n';
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: " << error.what() << '\n';
        draw_lightmap_generator::printUsage(std::cerr);
        return 2;
    }
    try
    {
        SignalScope signals;
        auto delegate = std::make_shared<BakeApplication>(std::move(options));
        bg2e::app::OffscreenApplication application;
        application.init(argc, argv, "org.bg2e.apps.draw.lightmap-generator", delegate);
        const int status = application.run(); // Includes complete native cleanup.
        if (status != 0)
        {
            return status;
        }
        const auto& result = delegate->result();
        if (result.cancelled)
        {
            std::cerr << "lightmap_generator: cancelled; no new batch was published\n";
            return interrupted == SIGTERM ? 143 : 130;
        }
        std::cout << "Baked " << result.bakedCount << " target(s). Manifest: " << result.manifestPath.string() << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: " << error.what() << '\n';
        return 1;
    }
}
