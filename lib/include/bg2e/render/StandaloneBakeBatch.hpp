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

#include <bg2e/db/LightmapOutputWriter.hpp>
#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>
#include <bg2e/render/LightmapSettings.hpp>
#include <bg2e/render/StandaloneBakeSceneAssembler.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace bg2e {
namespace render {

class Engine;

class BG2E_API StandaloneBakeBatch final {
public:
    struct Options {
        LightmapSettings lightmapSettings{};
        bool generateUv2 = false;
        uint32_t uv2PaddingPixels = 4;
        bool overwriteOutputs = false;
    };

    struct Progress {
        uint32_t targetIndex = 0; // Zero-based index among eligible bake targets.
        uint32_t targetCount = 0;
        uint32_t completedFrames = 0;
        uint32_t accumulationFrames = 0;
    };

    using ProgressCallback = std::function<bool(const Progress&)>;

    struct SkippedTarget {
        std::string identity;
        std::string reason;
    };

    struct Result {
        uint32_t bakedCount = 0;
        uint32_t skippedCount = 0;
        bool cancelled = false;
        std::vector<SkippedTarget> skippedTargets;
    };

    using WarningCallback = std::function<void(const SkippedTarget&)>;

    [[nodiscard]] static Result run(
        Engine* engine,
        const StandaloneBakeSceneAssembler::Assembly& assembly,
        const std::filesystem::path& outputDirectory,
        db::ImageFormat imageFormat,
        const Options& options,
        ProgressCallback onProgress = {},
        WarningCallback onWarning = {});
};

}
}
