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

#pragma once

// Add apps/lightmap_generator/src to the local driver's include path and
// compile CommandLine.cpp with it. This probe creates no Engine/GPU resources.
#include <CommandLine.hpp>
#include <bg2e/draw/SettingsCodec.hpp>
#include <fstream>
#include <stdexcept>

namespace draw_lightmap_generator::tests {
inline void requireCli(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

inline Options parseCli(std::vector<std::string> arguments)
{
    std::vector<char*> argv;
    for (auto& argument : arguments)
    {
        argv.push_back(argument.data());
    }
    return parseOptions(static_cast<int>(argv.size()), argv.data());
}

template <typename F> void rejectCli(F operation)
{
    bool rejected = false;
    try
    {
        operation();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    requireCli(rejected, "Invalid CLI/configuration was accepted");
}

// scratch must be a new, caller-owned directory outside protected/source assets.
inline void runDrawLightmapCliProbe(const std::filesystem::path& scratch)
{
    const std::vector<std::string> base{"draw_lightmap_generator", "model", "--context", "context.json",
        "--model", "model.bg2", "--output", "out"};
    auto parseExtra = [&](std::vector<std::string> extra)
    {
        auto args = base;
        args.insert(args.end(), extra.begin(), extra.end());
        return parseCli(std::move(args));
    };
    const auto defaults = parseCli(base);
    requireCli(defaults.generateUv2 && defaults.batch.overwriteOutputs && defaults.batch.writeModelCopies &&
        defaults.batch.settings.accumulationFrames() == 16, "Useful legacy defaults changed");
    const auto existing = parseExtra({"--generate-uv2=false", "--write-model-copies=true", "--overwrite=false"});
    requireCli(!existing.generateUv2 && existing.batch.writeModelCopies && !existing.batch.overwriteOutputs,
        "UV2/model/overwrite controls are coupled incorrectly");
    for (const auto& extra : std::vector<std::vector<std::string>>{
        {"--frames", "0"}, {"--frames", "4294967296"}, {"--frames", "1junk"},
        {"--resolution", "-1"}, {"--resolution", "1", "--dilation", "2"},
        {"--ray-bias", "nan"}, {"--overwrite", "yes"}, {"--frames=2", "--frames", "3"},
        {"--unknown"}, {"--prefab", "other.json"}, {"--backend", "opengl"},
        {"--write-model-copies", "false"}, {"--unknown-settings", "skip"},
        {"--mode", "rtao", "--gi-bounces", "2"}, {"--mode", "rtgi", "--ao-radius", "1"},
        {"--cpu-format", "rgb32f", "--format", "png"}, {"--cpu-format", "rgb32f", "--exposure", "2"},
        {"--generate-uv2", "false", "--uv2-padding", "0"}, {"--output="}})
    {
        rejectCli([&]
        {
            parseExtra(extra);
        });
    }
    std::filesystem::create_directories(scratch);
    const auto config = scratch / "settings.json";
    bg2e::bake::LightmapSettings settings;
    settings.setResolution(64);
    settings.setAccumulationFrames(3);
    bg2e::draw::SettingsCatalog catalog;
    catalog.reset("bg2e.bake.standalone.v1");
    bg2e::draw::SettingsTarget target;
    target.id = "bake";
    target.type = settings.settingsType();
    target.instance = settings.settingsInstance();
    catalog.add(std::move(target));
    auto document = bg2e::draw::SettingsCodec::serialize(catalog);
    auto save = [&]
    {
        std::ofstream out(config);
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out << bg2e::draw::SettingsCodec::stringify(*document);
    };
    save();
    const auto imported = parseExtra({"--frames", "7", "--config", config.string(), "--resolution", "32"});
    requireCli(imported.batch.settings.accumulationFrames() == 7 && imported.batch.settings.resolution() == 32 &&
        imported.batch.protectedInputs.size() == 1, "CLI overrides/config protection failed");
    using namespace bg2e::json;
    document->objectValue().at("components")->listValue().push_back(JSON(JsonObject{
        {"id", JSON("rendererOnly")}, {"type", JSON("unavailable")},
        {"properties", JSON(JsonObject{{"enabled", JSON(JsonObject{{"type", JSON("bool")}, {"value", JSON(true)}})}})}}));
    save();
    rejectCli([&]
    {
        parseExtra({"--config", config.string()});
    });
    const auto skipped = parseExtra({"--config", config.string(), "--unknown-settings", "skip"});
    requireCli(!skipped.diagnostics.empty(), "Skipped unavailable settings need diagnostics");
    document->objectValue().at("renderer") = JSON("renderer.deferred");
    save();
    rejectCli([&]
    {
        parseExtra({"--config", config.string(), "--unknown-settings", "skip"});
    });
}
}
