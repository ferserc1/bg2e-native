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

#include <bg2e.hpp>

#include <charconv>
#include <cmath>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
using namespace bg2e;
namespace fs = std::filesystem;

struct Options {
    fs::path outputDirectory = "offscreen_output";
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t frames = 1;
    gpu::BackendType backend = draw::EngineConfig{}.backend;
    std::optional<gpu::Size2D> internalExtent;
    std::optional<float> scale;
    fs::path graphicsConfig;
    draw::UnknownSettingPolicy unknownSettings = draw::UnknownSettingPolicy::Reject;
    bool help = false;
};

uint32_t parseUnsigned(std::string_view value, const char* option)
{
    uint32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data()+value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size() || result == 0)
    {
        throw std::invalid_argument(std::string(option)+" expects a positive integer");
    }
    return result;
}

float parseScale(std::string_view value)
{
    std::string copy(value);
    char* end = nullptr;
    const float result = std::strtof(copy.c_str(), &end);
    if (end != copy.c_str()+copy.size() || !std::isfinite(result) || result < 0.25f || result > 1.0f)
    {
        throw std::invalid_argument("--scale expects a number between 0.25 and 1");
    }
    return result;
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool widthSet = false, heightSet = false, internalWidthSet = false, internalHeightSet = false;
    uint32_t internalWidth = 0, internalHeight = 0;
    std::set<std::string_view> seen;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view option(argv[i]);
        if (option != "--help" && option != "-h" && !seen.insert(option).second)
        {
            throw std::invalid_argument("Option supplied more than once: "+std::string(option));
        }
        auto value = [&]() -> std::string_view
        {
            if (i+1 >= argc)
            {
                throw std::invalid_argument(std::string(option)+" requires a value");
            }
            return argv[++i];
        };
        if (option == "--help" || option == "-h")
        {
            options.help = true;
        }
        else if (option == "--output-dir")
        {
            options.outputDirectory = value();
        }
        else if (option == "--width")
        {
            options.width = parseUnsigned(value(), "--width");
            widthSet = true;
        }
        else if (option == "--height")
        {
            options.height = parseUnsigned(value(), "--height");
            heightSet = true;
        }
        else if (option == "--internal-width")
        {
            internalWidth = parseUnsigned(value(), "--internal-width");
            internalWidthSet = true;
        }
        else if (option == "--internal-height")
        {
            internalHeight = parseUnsigned(value(), "--internal-height");
            internalHeightSet = true;
        }
        else if (option == "--scale")
        {
            options.scale = parseScale(value());
        }
        else if (option == "--frames")
        {
            options.frames = parseUnsigned(value(), "--frames");
            if (options.frames > 1048576)
            {
                throw std::invalid_argument("--frames must not exceed 1048576");
            }
        }
        else if (option == "--backend")
        {
            const auto backend = value();
            if (backend == "vulkan")
            {
                options.backend = gpu::BackendType::Vulkan;
            }
            else if (backend == "metal")
            {
                options.backend = gpu::BackendType::Metal;
            }
            else
            {
                throw std::invalid_argument("--backend expects vulkan or metal");
            }
        }
        else if (option == "--graphics-config")
        {
            options.graphicsConfig = value();
        }
        else if (option == "--skip-unknown-settings")
        {
            options.unknownSettings = draw::UnknownSettingPolicy::SkipUnknownOrUnavailable;
        }
        else
        {
            throw std::invalid_argument("Unknown option: "+std::string(option));
        }
    }
    if (widthSet != heightSet)
    {
        throw std::invalid_argument("--width and --height must be supplied together");
    }
    if (internalWidthSet != internalHeightSet)
    {
        throw std::invalid_argument("--internal-width and --internal-height must be supplied together");
    }
    if (internalWidthSet)
    {
        options.internalExtent = gpu::Size2D{internalWidth, internalHeight};
    }
    if (options.scale && options.internalExtent)
    {
        throw std::invalid_argument("--scale and explicit internal dimensions are mutually exclusive");
    }
    if (options.outputDirectory.empty())
    {
        throw std::invalid_argument("--output-dir must not be empty");
    }
    return options;
}

void printUsage(const char* executable)
{
    std::cout << "Usage: " << executable << " [options]\n"
        << "  --output-dir DIR                 destination directory (default offscreen_output)\n"
        << "  --width N --height N             output dimensions (default 1920x1080)\n"
        << "  --internal-width N --internal-height N\n"
        << "                                   fixed internal dimensions\n"
        << "  --scale N                        internal scale from 0.25 to 1\n"
        << "  --frames N                       completed samples, 1 to 1048576\n"
        << "  --backend vulkan|metal           GPU backend\n"
        << "  --graphics-config FILE           exported draw settings JSON\n"
        << "  --skip-unknown-settings          skip unavailable/unknown exported targets\n"
        << "  --help                           show this help\n"
        << "Writes scene.png and refuses to overwrite an existing file.\n";
}

fs::path pendingPngPath(const fs::path& destination)
{
    auto pending = destination;
    pending.replace_filename(destination.stem().string()+".pending"+destination.extension().string());
    return pending;
}

void saveVerifiedPng(const fs::path& destination, const std::vector<uint8_t>& pixels,
    uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 ||
        size_t(width) > std::numeric_limits<size_t>::max() / 4 / size_t(height) ||
        pixels.size() != size_t(width) * size_t(height) * 4)
    {
        throw std::runtime_error("Completed RGBA8 image has an invalid byte count");
    }

    const auto temporary = pendingPngPath(destination);
    if (fs::exists(temporary))
    {
        throw std::runtime_error("Temporary output already exists: "+temporary.string());
    }

    try
    {
        db::saveImage(temporary, pixels.data(), width, height, 4);
        const auto encodedSize = fs::file_size(temporary);
        if (encodedSize == 0 || encodedSize > static_cast<uintmax_t>(std::numeric_limits<int>::max()))
        {
            throw std::runtime_error("Encoded PNG has an invalid size");
        }
        std::vector<uint8_t> encoded(static_cast<size_t>(encodedSize));
        std::ifstream input(temporary, std::ios::binary);
        if (!input || !input.read(reinterpret_cast<char*>(encoded.data()), static_cast<std::streamsize>(encoded.size())))
        {
            throw std::runtime_error("Could not read the encoded PNG for verification");
        }

        uint32_t decodedWidth = 0;
        uint32_t decodedHeight = 0;
        const auto decoded = db::decodeImageRGBA8(encoded.data(), encoded.size(), decodedWidth, decodedHeight);
        if (decodedWidth != width || decodedHeight != height || decoded != pixels)
        {
            throw std::runtime_error("Encoded PNG does not preserve the completed RGBA8 image");
        }
        fs::create_hard_link(temporary, destination);
        std::error_code ignored;
        fs::remove(temporary, ignored);
    }
    catch (...)
    {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}

class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width/_viewport.height, near(), far());
    }
};

class OffscreenScene final : public draw::DefaultOffscreenApplicationDelegate {
  public:
    OffscreenScene()
        : DefaultOffscreenApplicationDelegate(makeRenderer(), makeConfiguration())
    {
    }

    void initConfig(int argc, char** argv, app::DrawOffscreenApplicationConfig& config) override
    {
        _options = parseOptions(argc, argv);
        if (_options.help)
        {
            return;
        }
        config.extent = {_options.width, _options.height};
        config.engine.colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
        config.engine.backend = _options.backend;
        config.frames = _options.frames;
        config.graphicsConfig = _options.graphicsConfig;
        config.unknownSettings = _options.unknownSettings;
        config.internalExtentOverride = _options.internalExtent;
        config.scaleOverride = _options.scale;
        _outputFile = fs::absolute(_options.outputDirectory).lexically_normal()/"scene.png";
        fs::create_directories(_outputFile.parent_path());
        if (fs::exists(_outputFile) || fs::exists(pendingPngPath(_outputFile)))
        {
            throw std::runtime_error("Output or temporary output already exists in: "+_outputFile.parent_path().string());
        }
    }

    void cleanup() override
    {
        std::exception_ptr error;
        try
        {
            DefaultOffscreenApplicationDelegate::cleanup();
        }
        catch (...)
        {
            error = std::current_exception();
        }
        for (auto& drawable : _drawables)
        {
            try
            {
                drawable->cleanup();
            }
            catch (...)
            {
                if (!error)
                {
                    error = std::current_exception();
                }
            }
        }
        _drawables.clear();
        if (error)
        {
            std::rethrow_exception(error);
        }
    }

    void didRenderFrame(uint64_t frame, double) override
    {
        if (frame+1 != _options.frames || completedFrames() != _options.frames || !hasResult())
        {
            return;
        }
        auto* surface = _engine->offscreenSurface();
        auto* image = surface ? surface->colorImage(0) : nullptr;
        if (!image || image->size() != gpu::Size2D{_options.width,_options.height} ||
            image->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM)
        {
            throw std::runtime_error("Completed offscreen image has an unexpected format or extent");
        }
        std::vector<uint8_t> pixels;
        image->readPixelsRGBA8(pixels, gpu::ImageLayout::TransferSrc);
        saveVerifiedPng(_outputFile, pixels, _options.width, _options.height);
        std::cout << "Rendered " << completedFrames() << " samples to " << _outputFile.string() << '\n';
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Offscreen deferred scene");
        root->addComponent(new scene::EnvironmentComponent(base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));

        auto model = std::make_shared<draw::Drawable>();
        model->setMesh(db::loadMeshObj<draw::Drawable::Mesh>(base::PlatformTools::assetPath()/"two_submeshes.obj"));
        const char* names[] = {"outer", "inner"};
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto texture = [&](const std::string& map)
            {
                return std::make_shared<base::Texture>(base::PlatformTools::assetPath(),
                    "two_submeshes_"+std::string(names[i])+"_"+map+".jpg");
            };
            auto& material = model->material(i);
            material.setAlbedoTexture(texture("albedo"));
            material.setMetalnessTexture(texture("metallic"));
            material.setRoughnessTexture(texture("roughness"));
            material.setNormalTexture(texture("normal"));
            material.setAoTexture(texture("ao"));
            material.setMetalness(1);
            material.setRoughness(1);
            material.setAlphaCutoff(0);
            model->setSubmeshName(names[i], i);
        }
        model->material(1).setFresnelTint(base::Color(0.8f, 0.2f, 1));
        model->material(1).setSheenColor(base::Color::Blue());
        model->material(1).setSheenIntensity(0.5f);
        load(model);
        addDrawable(root, "Textured model", model, {0,0,0});

        auto glass = model->clone();
        glass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(1,1,1,0.45f));
            material.setIsTransparent(true);
            material.setRefractionFactor(0.1f);
        });
        _drawables.push_back(glass);
        addDrawable(root, "Transparent model", glass, {3,0,0});

        auto solidGlass = model->clone();
        solidGlass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(0.7f,1,0.7f,0.5f));
            material.setIsTransparent(true);
            material.setIsSolid(true);
        });
        _drawables.push_back(solidGlass);
        addDrawable(root, "Solid transparent model", solidGlass, {1.5f,0,2.5f});

        auto floor = std::make_shared<draw::Drawable>();
        floor->setMesh(geo::createPlane(30,30));
        floor->material().setAlbedo(base::Color(0.55f,0.55f,0.55f));
        floor->material().setMetalness(0);
        floor->material().setAlphaCutoff(0);
        floor->material().setRoughness(0.75f);
        load(floor);
        addDrawable(root, "Floor", floor, {0,-1.05f,0});

        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(0,5,13), glm::vec3(0,0.5f,0), glm::vec3(0,1,0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        root->addChild(camera);

        auto lights = std::make_shared<scene::Node>("Lights");
        lights->addComponent(new scene::TransformComponent());
        root->addChild(lights);
        for (uint32_t i = 0; i < 4; ++i)
        {
            auto light = std::make_shared<scene::Node>("Light "+std::to_string(i+1));
            const float angle = float(i)*1.57079633f;
            light->addComponent(scene::TransformComponent::makeTranslated(std::sin(angle)*5,5,std::cos(angle)*5));
            light->addComponent(new scene::LightComponent());
            auto& value = light->light()->light();
            value.setType(base::Light::TypeOmni);
            value.setColor(base::Color(i == 0 ? 1 : 0.65f, i == 1 ? 1 : 0.7f, i == 2 ? 1 : 0.75f));
            value.setIntensity(35);
            value.setCastShadows(true);
            value.setSourceSize(0.25f);
            value.setShadowSamples(8);
            lights->addChild(light);
        }
        return root;
    }

  private:
    static draw::RendererConfiguration makeConfiguration()
    {
        draw::RendererConfiguration configuration;
        configuration.resolution = std::make_shared<draw::ResolutionSettings>();
        // Let the renderer retain the native backend upscaler, initially disabled.
        configuration.finalProcessor = std::make_shared<draw::pipeline::SMAAProcessor>();
        return configuration;
    }

    static std::unique_ptr<draw::Renderer> makeRenderer()
    {
        return std::make_unique<draw::RendererDeferred>();
    }

    void load(const std::shared_ptr<draw::Drawable>& drawable)
    {
        _drawables.push_back(drawable);
        drawable->load(_engine);
    }

    std::shared_ptr<scene::Node> addDrawable(const std::shared_ptr<scene::Node>& parent,
        const std::string& name, std::shared_ptr<draw::Drawable> drawable, glm::vec3 position)
    {
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(scene::TransformComponent::makeTranslated(position));
        node->addComponent(new scene::DrawableComponent(std::move(drawable)));
        parent->addChild(node);
        return node;
    }

    Options _options;
    fs::path _outputFile;
    std::vector<std::shared_ptr<draw::Drawable>> _drawables;
};
}

int main(int argc, char** argv)
{
    try
    {
        auto options = parseOptions(argc, argv);
        if (options.help)
        {
            printUsage(argv[0]);
            return 0;
        }
        auto delegate = std::make_shared<OffscreenScene>();
        app::OffscreenApplication application;
        application.init(argc, argv, "org.bg2e.examples.draw.offscreen-application", delegate);
        for (const auto& issue : delegate->settingsResult().issues)
        {
            if (!issue.error)
            {
                std::cerr << "Skipped graphics setting " << issue.path << ": " << issue.message << '\n';
            }
        }
        application.run();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_offscreen_application: " << error.what() << '\n';
        return 1;
    }
}
