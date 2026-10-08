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
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace lightmap_generator {
namespace {

std::string asciiLower(std::string_view value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value)
    {
        if (character >= 'A' && character <= 'Z')
        {
            character = static_cast<unsigned char>(character + ('a' - 'A'));
        }
        result.push_back(static_cast<char>(character));
    }
    return result;
}

uint32_t parsePositiveUint32(std::string_view value, const std::string& optionName)
{
    uint64_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed, 10);
    if (value.empty() || result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        parsed == 0 || parsed > std::numeric_limits<uint32_t>::max())
    {
        throw std::invalid_argument(optionName + " must be a positive 32-bit integer");
    }
    return static_cast<uint32_t>(parsed);
}

float parsePositiveFloat(std::string_view value, const std::string& optionName)
{
    if (value.empty())
    {
        throw std::invalid_argument(optionName + " must be a finite positive number");
    }
    const std::string text(value);
    char* end = nullptr;
    errno = 0;
    const float parsed = std::strtof(text.c_str(), &end);
    if (errno == ERANGE || end != text.c_str() + text.size() || !std::isfinite(parsed) || parsed <= 0.0f)
    {
        throw std::invalid_argument(optionName + " must be a finite positive number");
    }
    return parsed;
}

bool parseBool(std::string_view value, const std::string& optionName)
{
    const auto normalized = asciiLower(value);
    if (normalized == "true") return true;
    if (normalized == "false") return false;
    throw std::invalid_argument(optionName + " must be true or false");
}

std::string optionValue(
    int& index,
    int argc,
    char** argv,
    std::string_view token,
    size_t equalsPosition,
    const std::string& optionName)
{
    if (equalsPosition != std::string_view::npos)
    {
        const auto value = token.substr(equalsPosition + 1);
        if (value.empty())
        {
            throw std::invalid_argument(optionName + " requires a value");
        }
        return std::string(value);
    }

    if (index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("--"))
    {
        throw std::invalid_argument(optionName + " requires a value");
    }
    return argv[++index];
}

void requireRegularFile(const std::filesystem::path& path, const std::string& description)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error)
    {
        throw std::invalid_argument(description + " file does not exist or is not a regular file: '" +
            path.string() + "'");
    }
}

void validateOutputDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error)
    {
        throw std::invalid_argument("Could not inspect output directory '" + path.string() + "': " +
            error.message());
    }
    if (exists && !std::filesystem::is_directory(path, error))
    {
        throw std::invalid_argument("Output path is not a directory: '" + path.string() + "'");
    }
    if (error)
    {
        throw std::invalid_argument("Could not inspect output directory '" + path.string() + "': " +
            error.message());
    }

    auto parent = path.parent_path();
    if (parent.empty()) parent = ".";
    const bool parentExists = std::filesystem::exists(parent, error);
    if (error)
    {
        throw std::invalid_argument("Could not inspect output parent directory '" + parent.string() + "': " +
            error.message());
    }
    if (parentExists && !std::filesystem::is_directory(parent))
    {
        throw std::invalid_argument("Output parent is not a directory: '" + parent.string() + "'");
    }
}

}

bool helpRequested(int argc, char** argv)
{
    size_t helpCount = 0;
    for (int index = 1; index < argc; ++index)
    {
        if (std::string_view(argv[index]) == "--help")
        {
            ++helpCount;
        }
    }
    return helpCount == 1;
}

Options parseOptions(int argc, char** argv)
{
    if (argc < 2)
    {
        throw std::invalid_argument("A model or prefab subcommand is required");
    }

    Options options;
    options.batch.generateUv2 = true;
    options.batch.overwriteOutputs = true;
    const std::string_view command(argv[1]);
    if (command == "model")
    {
        options.command = Command::Model;
    }
    else if (command == "prefab")
    {
        options.command = Command::Prefab;
    }
    else
    {
        throw std::invalid_argument("Subcommand must be 'model' or 'prefab'");
    }

    std::set<std::string> seenOptions;
    for (int index = 2; index < argc; ++index)
    {
        const std::string_view token(argv[index]);
        if (!token.starts_with("--"))
        {
            throw std::invalid_argument("Unexpected positional argument '" + std::string(token) + "'");
        }

        const auto equalsPosition = token.find('=');
        const std::string optionName(token.substr(0, equalsPosition));
        if (!seenOptions.insert(optionName).second)
        {
            throw std::invalid_argument("Option '" + optionName + "' was specified more than once");
        }
        if (optionName == "--help")
        {
            throw std::invalid_argument("--help does not accept a value");
        }

        const auto value = optionValue(index, argc, argv, token, equalsPosition, optionName);
        if (optionName == "--context")
        {
            options.contextPath = value;
        }
        else if (optionName == "--model")
        {
            options.modelPath = value;
        }
        else if (optionName == "--prefab")
        {
            options.prefabPath = value;
        }
        else if (optionName == "--output")
        {
            options.outputDirectory = value;
        }
        else if (optionName == "--format")
        {
            const auto format = bg2e::db::imageFormatFromExtension(value);
            if (!format)
            {
                throw std::invalid_argument("Unsupported image format '" + value + "'");
            }
            options.imageFormat = *format;
        }
        else if (optionName == "--resolution")
        {
            options.batch.lightmapSettings.resolution = parsePositiveUint32(value, optionName);
        }
        else if (optionName == "--frames")
        {
            options.batch.lightmapSettings.accumulationFrames = parsePositiveUint32(value, optionName);
        }
        else if (optionName == "--mode")
        {
            const auto mode = asciiLower(value);
            if (mode == "rtao")
            {
                options.batch.lightmapSettings.mode = bg2e::render::LightmapMode::RTAO;
            }
            else if (mode == "rtgi")
            {
                options.batch.lightmapSettings.mode = bg2e::render::LightmapMode::RTGI;
            }
            else
            {
                throw std::invalid_argument("--mode must be 'rtao' or 'rtgi'");
            }
        }
        else if (optionName == "--generate-uv2")
        {
            options.batch.generateUv2 = parseBool(value, optionName);
        }
        else if (optionName == "--overwrite")
        {
            options.batch.overwriteOutputs = parseBool(value, optionName);
        }
        else if (optionName == "--samples-per-pixel")
        {
            options.batch.lightmapSettings.samplesPerPixel = parsePositiveUint32(value, optionName);
        }
        else if (optionName == "--gi-bounces")
        {
            options.batch.lightmapSettings.giBounces = parsePositiveUint32(value, optionName);
            options.giBouncesSpecified = true;
        }
        else if (optionName == "--max-distance")
        {
            options.batch.lightmapSettings.maxRayDistance = parsePositiveFloat(value, optionName);
            options.maxDistanceSpecified = true;
        }
        else
        {
            throw std::invalid_argument("Unknown option '" + optionName + "'");
        }
    }

    if (options.contextPath.empty() || options.outputDirectory.empty())
    {
        throw std::invalid_argument("--context and --output are required");
    }
    if (options.command == Command::Model)
    {
        if (options.modelPath.empty() || !options.prefabPath.empty())
        {
            throw std::invalid_argument("Model mode requires --model and does not accept --prefab");
        }
    }
    else if (options.prefabPath.empty() || !options.modelPath.empty())
    {
        throw std::invalid_argument("Prefab mode requires --prefab and does not accept --model");
    }

    if (options.batch.lightmapSettings.mode == bg2e::render::LightmapMode::RTAO &&
        (options.giBouncesSpecified || options.maxDistanceSpecified))
    {
        throw std::invalid_argument("--gi-bounces and --max-distance are only valid with --mode rtgi");
    }

    return options;
}

void validateOptions(const Options& options)
{
    requireRegularFile(options.contextPath, "Context");
    if (options.command == Command::Model)
    {
        requireRegularFile(options.modelPath, "Model");
        if (asciiLower(options.modelPath.extension().string()) != ".bg2")
        {
            throw std::invalid_argument("--model must name a .bg2 file");
        }
    }
    else
    {
        requireRegularFile(options.prefabPath, "Prefab");
    }
    validateOutputDirectory(options.outputDirectory);
}

void printUsage(std::ostream& output)
{
    output <<
        "Usage:\n"
        "  lightmap_generator model --context <scene.json> --model <target.bg2> --output <directory> [options]\n"
        "  lightmap_generator prefab --context <scene.json> --prefab <target.json> --output <directory> [options]\n"
        "\n"
        "Options (values may use --name=value or --name value):\n"
        "  --format <png|jpg|jpeg|bmp|tga>  Image format (default: png)\n"
        "  --resolution <pixels>             Square bake/UV2 resolution (default: 512)\n"
        "  --frames <count>                  Accumulation frames (default: 16)\n"
        "  --mode <rtao|rtgi>                Bake mode (default: rtgi)\n"
        "  --generate-uv2 <true|false>       Generate UV2 and write .bg2 copies (default: true)\n"
        "  --overwrite <true|false>          Overwrite existing output files (default: true)\n"
        "  --samples-per-pixel <count>       Samples per pixel (default: 8)\n"
        "  --gi-bounces <count>              RTGI bounces (default: 2; RTGI only)\n"
        "  --max-distance <meters>           RTGI ray distance (default: 50; RTGI only)\n"
        "  --help                            Show this help\n";
}

}
