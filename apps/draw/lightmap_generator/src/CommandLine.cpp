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
#include <charconv>
#include <cmath>
#include <map>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace draw_lightmap_generator {
using namespace bg2e;
namespace {

std::string lower(std::string value)
{
    for (auto& c : value)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast<char>(c + ('a' - 'A'));
        }
    }
    return value;
}

uint32_t integer(const std::string& value)
{
    uint32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
    {
        throw std::invalid_argument("Expected a nonnegative 32-bit integer: " + value);
    }
    return result;
}

float number(const std::string& value)
{
    float result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(result))
    {
        throw std::invalid_argument("Expected a finite number: " + value);
    }
    return result;
}

bool boolean(const std::string& value)
{
    const auto v = lower(value);
    if (v != "true" && v != "false")
    {
        throw std::invalid_argument("Expected true or false: " + value);
    }
    return v == "true";
}

void regular(const std::filesystem::path& p, const char* label)
{
    if (p.empty() || !std::filesystem::is_regular_file(p))
    {
        throw std::invalid_argument(std::string(label) + " must name an existing regular file: " + p.string());
    }
}

void importSettings(Options& options, draw::UnknownSettingPolicy policy)
{
    draw::SettingsCatalog catalog;
    catalog.reset("bg2e.bake.standalone.v1");
    draw::SettingsTarget target;
    target.id = "bake";
    target.type = options.batch.settings.settingsType();
    target.instance = options.batch.settings.settingsInstance();
    catalog.add(std::move(target));
    const auto document = draw::SettingsCodec::readFile(options.configPath);
    const auto plan = draw::SettingsCodec::validate(*document, catalog, policy);
    options.diagnostics = plan.result().issues;
    if (!plan.result().valid())
    {
        std::string message = "Invalid bake settings configuration";
        for (const auto& issue : options.diagnostics)
        {
            message += "\n  " + issue.path + ": " + issue.message;
        }
        throw std::invalid_argument(message);
    }
    const auto applied = plan.apply(catalog);
    if (!applied.valid() || !applied.applied)
    {
        throw std::invalid_argument("Could not apply bake settings configuration");
    }
    options.batch.protectedInputs.push_back(options.configPath);
}
}

bool helpRequested(int argc, char** argv)
{
    // Help is a standalone action; it must not conceal malformed bake arguments.
    return argc == 2 && std::string_view(argv[1]) == "--help";
}

Options parseOptions(int argc, char** argv)
{
    if (argc < 2)
    {
        throw std::invalid_argument("A model or prefab subcommand is required");
    }
    Options options;
    options.batch.overwriteOutputs = true; // Existing CLI behavior, documented explicitly.
    const std::string_view command(argv[1]);
    if (command == "prefab")
    {
        options.command = Command::Prefab;
    }
    else if (command != "model")
    {
        throw std::invalid_argument("Subcommand must be model or prefab");
    }
    const std::set<std::string> known{
        "--context", "--model", "--prefab", "--output", "--format", "--resolution", "--frames",
        "--mode", "--generate-uv2", "--overwrite", "--samples-per-pixel", "--gi-bounces", "--max-distance",
        "--backend", "--config", "--unknown-settings", "--cpu-format", "--exposure", "--ao-radius",
        "--ray-bias", "--dilation", "--uv2-padding", "--write-model-copies", "--shader-root", "--debug"};
    std::map<std::string, std::string> values;
    for (int i = 2; i < argc; ++i)
    {
        const std::string_view token(argv[i]);
        const auto equals = token.find('=');
        const std::string name(token.substr(0, equals));
        if (!known.contains(name))
        {
            throw std::invalid_argument("Unknown option: " + name);
        }
        std::string value;
        if (equals != std::string_view::npos)
        {
            value = token.substr(equals + 1);
        }
        else if (i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("--"))
        {
            value = argv[++i];
        }
        if (value.empty() || !values.emplace(name, value).second)
        {
            throw std::invalid_argument("Missing value or duplicate option: " + name);
        }
    }
    const auto get = [&](const std::string& name) -> std::string
    {
        const auto found = values.find(name);
        return found == values.end() ? std::string{} : found->second;
    };
    options.contextPath = get("--context");
    options.outputDirectory = get("--output");
    options.inputPath = get(options.command == Command::Model ? "--model" : "--prefab");
    if (options.contextPath.empty() || options.outputDirectory.empty() || options.inputPath.empty() ||
        values.contains(options.command == Command::Model ? "--prefab" : "--model"))
    {
        throw std::invalid_argument("Require --context, --output and exactly the subcommand's --model or --prefab");
    }
    auto policy = draw::UnknownSettingPolicy::Reject;
    if (values.contains("--unknown-settings"))
    {
        const auto value = get("--unknown-settings");
        if (value != "reject" && value != "skip")
        {
            throw std::invalid_argument("--unknown-settings must be reject or skip");
        }
        policy = value == "skip" ? draw::UnknownSettingPolicy::SkipUnknownOrUnavailable : policy;
        if (!values.contains("--config"))
        {
            throw std::invalid_argument("--unknown-settings requires --config");
        }
    }
    if (values.contains("--config"))
    {
        options.configPath = get("--config");
        regular(options.configPath, "Configuration");
        importSettings(options, policy);
    }
    // Apply explicit scalar overrides AFTER the imported live settings object.
    auto& s = options.batch.settings;
    for (const auto& [name, value] : values)
    {
        if (name == "--resolution")
        {
            s.setResolution(integer(value));
        }
        else if (name == "--frames")
        {
            s.setAccumulationFrames(integer(value));
        }
        else if (name == "--samples-per-pixel")
        {
            s.setSamplesPerPixel(integer(value));
        }
        else if (name == "--gi-bounces")
        {
            s.setGiBounces(integer(value));
        }
        else if (name == "--max-distance")
        {
            s.setMaxRayDistance(number(value));
        }
        else if (name == "--ao-radius")
        {
            s.setAoRadius(number(value));
        }
        else if (name == "--ray-bias")
        {
            s.setRayBias(number(value));
        }
        else if (name == "--exposure")
        {
            s.setExposureEV(number(value));
        }
        else if (name == "--dilation")
        {
            s.setDilationPixels(integer(value));
        }
        else if (name == "--uv2-padding")
        {
            options.uv2Padding = integer(value);
        }
        else if (name == "--generate-uv2")
        {
            options.generateUv2 = boolean(value);
        }
        else if (name == "--overwrite")
        {
            options.batch.overwriteOutputs = boolean(value);
        }
        else if (name == "--debug")
        {
            options.engine.debug = boolean(value);
        }
        else if (name == "--shader-root")
        {
            options.batch.shaderRoot = value;
        }
        else if (name == "--format")
        {
            const auto format = db::imageFormatFromExtension(value);
            if (!format)
            {
                throw std::invalid_argument("Unsupported byte image format: " + value);
            }
            options.batch.byteFormat = *format;
        }
        else if (name == "--mode")
        {
            const auto mode = lower(value);
            if (mode != "rtao" && mode != "rtgi")
            {
                throw std::invalid_argument("--mode must be rtao or rtgi");
            }
            s.setMode(mode == "rtao" ? bake::LightmapMode::RTAO : bake::LightmapMode::RTGI);
        }
        else if (name == "--cpu-format")
        {
            const auto format = lower(value);
            if (format != "rgb8" && format != "rgb32f")
            {
                throw std::invalid_argument("--cpu-format must be rgb8 or rgb32f");
            }
            s.setCpuFormat(format == "rgb8" ? bake::LightmapPixelFormat::RGB8 : bake::LightmapPixelFormat::RGB32F);
        }
        else if (name == "--backend")
        {
            const auto backend = lower(value);
            if (backend != "vulkan" && backend != "metal")
            {
                throw std::invalid_argument("--backend must be vulkan or metal");
            }
#ifndef BG2E_IS_MAC
            if (backend == "metal")
            {
                throw std::invalid_argument("Metal is only available on macOS");
            }
#endif
            options.engine.backend = backend == "metal" ? gpu::BackendType::Metal : gpu::BackendType::Vulkan;
        }
    }
    options.batch.writeModelCopies = values.contains("--write-model-copies") ?
        boolean(get("--write-model-copies")) : options.generateUv2;
    if (options.generateUv2 && !options.batch.writeModelCopies)
    {
        throw std::invalid_argument("Generated UV2 requires model copies to retain the generated atlas");
    }
    if (!options.generateUv2 && values.contains("--uv2-padding"))
    {
        throw std::invalid_argument("--uv2-padding requires --generate-uv2 true");
    }
    if (options.generateUv2 && options.uv2Padding > s.resolution())
    {
        throw std::invalid_argument("UV2 padding exceeds bake resolution");
    }
    if (s.cpuFormat() == bake::LightmapPixelFormat::RGB32F &&
        (values.contains("--format") || values.contains("--exposure")))
    {
        throw std::invalid_argument("RGB32F preserves HDR; --format and --exposure apply only to RGB8");
    }
    if ((s.mode() == bake::LightmapMode::RTAO &&
         (values.contains("--gi-bounces") || values.contains("--max-distance") || values.contains("--exposure"))) ||
        (s.mode() == bake::LightmapMode::RTGI && values.contains("--ao-radius")))
    {
        throw std::invalid_argument("Explicit AO/GI options must match the selected bake mode");
    }
    if (!options.batch.shaderRoot.empty())
    {
        options.batch.protectedInputs.push_back(options.batch.shaderRoot);
    }
    s.validate();
    return options;
}

void validateOptions(const Options& options)
{
    regular(options.contextPath, "Context");
    regular(options.inputPath, "Input");
    if (options.command == Command::Model)
    {
        const auto ext = lower(options.inputPath.extension().string());
        if (ext != ".bg2" && ext != ".vwglb" && ext != ".obj")
        {
            throw std::invalid_argument("Model must be .bg2, .vwglb or .obj");
        }
    }
    auto parent = std::filesystem::absolute(options.outputDirectory).lexically_normal();
    while (!parent.empty() && !std::filesystem::exists(parent))
    {
        parent = parent.parent_path();
    }
    if (parent.empty() || !std::filesystem::is_directory(parent))
    {
        throw std::invalid_argument("Output path has a non-directory ancestor");
    }
    if (!options.batch.shaderRoot.empty() &&
        (!std::filesystem::is_directory(options.batch.shaderRoot / "draw") ||
         !std::filesystem::is_directory(options.batch.shaderRoot / "bake")))
    {
        throw std::invalid_argument("--shader-root must contain deployed draw and bake shader directories");
    }
}

void printUsage(std::ostream& output)
{
    output << "Usage:\n"
        "  draw_lightmap_generator model --context scene.json --model target.bg2 --output directory [options]\n"
        "  draw_lightmap_generator prefab --context scene.json --prefab target.json --output directory [options]\n\n"
        "Every option requires a value (--name=value or --name value).\n"
        "  --backend vulkan|metal             Default: Metal on macOS, Vulkan elsewhere\n"
        "  --config path                      Typed SettingsCodec bake configuration\n"
        "  --unknown-settings reject|skip     Default: reject; skipped fields emit warnings\n"
        "  --mode rtao|rtgi                   Default: rtgi\n"
        "  --resolution pixels                Default: 512 (1..16384)\n"
        "  --frames count                     Default: 16 (completed UV samples)\n"
        "  --samples-per-pixel count           Default: 8 (1..64)\n"
        "  --gi-bounces count                  Default: 2 (1..8, GI only)\n"
        "  --max-distance meters              Default: 50 (GI only)\n"
        "  --ao-radius meters                 Default: 0.1 (AO only)\n"
        "  --ray-bias meters                  Default: 0.0005\n"
        "  --cpu-format rgb8|rgb32f            Default: rgb8; float uses exact HDR .bg2tex\n"
        "  --format png|jpg|jpeg|bmp|tga        RGB8 only; default: png; JPEG is lossy\n"
        "  --exposure EV                      Default: 0 (RGB8 GI conversion only)\n"
        "  --dilation pixels                  Default: 4 (0..resolution)\n"
        "  --generate-uv2 true|false           Default: true; requires model copies\n"
        "  --uv2-padding pixels               Default: 4 (generated atlas only)\n"
        "  --write-model-copies true|false     Default: generate-uv2 value\n"
        "  --overwrite true|false             Default: true; replace manifest, retain old generations\n"
        "  --shader-root directory            Default: deployed engine draw/bake shader root\n"
        "  --debug true|false                 GPU validation; default: false\n"
        "  --help                             Standalone help; no GPU initialization\n\n"
        "Model inputs: BG2/VWGLB/OBJ. CLI settings override configuration regardless of order.\n"
        "Success publishes lightmaps.json pointing to one complete generation.\n"
        "Exit codes: 0 success/help; 2 arguments/config; 1 initialization/bake/output/cleanup;\n"
        "130 SIGINT cancellation; 143 SIGTERM cancellation. Custom dependencies fail closed.\n";
}
}
