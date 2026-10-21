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

#include <bg2e/bake/LightmapOutputWriter.hpp>
#include <bg2e/bake/StandaloneBakeSceneAssembler.hpp>
#include <bg2e/bake/BakerContext.hpp>

namespace bg2e::bake {
class BG2E_API StandaloneBakeBatch final {
  public:
    struct Options {
        LightmapSettings settings;
        db::ImageFormat byteFormat = db::ImageFormat::PNG;
        bool overwriteOutputs = false, writeModelCopies = false;
        std::filesystem::path shaderRoot;
        std::shared_ptr<const draw::environment::EnvironmentResources> environment;
        std::vector<std::filesystem::path> protectedInputs;
        BakeDependencyResolver resolveDependencies;
        // Optional application job/document validity. CPU read-only, no GPU work.
        std::function<bool()> current;
    };
    struct Progress {
        uint32_t targetIndex = 0, targetCount = 0, completedSamples = 0, requestedSamples = 0;
    };
    struct Result {
        uint32_t bakedCount = 0; // Committed target count; zero on cancellation.
        bool cancelled = false;
        std::filesystem::path manifestPath;
        std::vector<LightmapOutputWriter::OutputPaths> outputs;
    };
    using ProgressCallback = std::function<bool(const Progress&)>;

    // Requires a private assembly and stopped producers. Deferred target resources
    // are realized before preflight. UV2 generation belongs to the assembler.
    // No map is applied to source materials between targets. Cancel returns no
    // publication; failure throws with no new committed generation.
    static Result run(draw::Engine& engine, const StandaloneBakeSceneAssembler::Assembly& assembly,
        const std::filesystem::path& outputDirectory, const Options& options,
        ProgressCallback onProgress = {});
};
}
