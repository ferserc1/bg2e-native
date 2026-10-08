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

#include <bg2e/db/LightmapOutputWriter.hpp>

#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/Node.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bg2e::db {
namespace {

using Path = std::filesystem::path;

std::atomic_uint64_t temporaryCounter{ 0 };

Path absoluteNormalized(const Path& path, const std::string& description)
{
    if (path.empty())
    {
        throw std::invalid_argument("LightmapOutputWriter: " + description + " path must not be empty");
    }

    std::error_code error;
    auto result = std::filesystem::absolute(path, error);
    if (error)
    {
        throw std::runtime_error("LightmapOutputWriter: could not resolve " + description + " path '" +
            path.string() + "': " + error.message());
    }
    return result.lexically_normal();
}

Path canonicalForComparison(const Path& path)
{
    std::error_code error;
    auto result = std::filesystem::weakly_canonical(path, error);
    if (error)
    {
        return path.lexically_normal();
    }
    return result.lexically_normal();
}

std::string pathKey(const Path& path)
{
    auto key = canonicalForComparison(path).generic_string();
#ifdef _WIN32
    for (auto& character : key)
    {
        if (character >= 'A' && character <= 'Z')
        {
            character = static_cast<char>(character + ('a' - 'A'));
        }
    }
#endif
    return key;
}

bool pathExistsIncludingSymlink(const Path& path)
{
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory)
    {
        return false;
    }
    if (error)
    {
        throw std::runtime_error("LightmapOutputWriter: could not inspect output path '" +
            path.string() + "': " + error.message());
    }
    return status.type() != std::filesystem::file_type::not_found;
}

std::string stableHash(std::string_view value)
{
    uint64_t hash = 14695981039346656037ull;
    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string safeStem(std::string_view identity)
{
    std::string result;
    result.reserve(identity.size());
    for (const unsigned char character : identity)
    {
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '_' || character == '-')
        {
            result.push_back(static_cast<char>(character));
        }
        else if (character == '/' || character == '\\')
        {
            result += "__";
        }
        else
        {
            result.push_back('_');
        }
    }
    while (!result.empty() && result.front() == '.')
    {
        result.erase(result.begin());
    }
    if (result.empty())
    {
        return "target";
    }
    if (result.size() > 128)
    {
        result = result.substr(0, 96) + "-" + stableHash(identity);
    }
    return result;
}

std::vector<uint8_t> rgb8Pixels(const render::LightmapPixels& pixels)
{
    if (pixels.width == 0 || pixels.height == 0 ||
        pixels.width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        pixels.height > static_cast<uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument("LightmapOutputWriter: lightmap dimensions must be positive and supported");
    }

    const auto width = static_cast<size_t>(pixels.width);
    const auto height = static_cast<size_t>(pixels.height);
    if (width > std::numeric_limits<size_t>::max() / height / 3)
    {
        throw std::invalid_argument("LightmapOutputWriter: lightmap dimensions overflow the pixel buffer size");
    }
    const size_t byteCount = width * height * 3;

    if (pixels.format == render::LightmapPixelFormat::RGB8)
    {
        const auto* values = std::get_if<std::vector<uint8_t>>(&pixels.rgb);
        if (!values || values->size() != byteCount)
        {
            throw std::invalid_argument("LightmapOutputWriter: RGB8 result has an invalid pixel buffer");
        }
        return *values;
    }

    if (pixels.format != render::LightmapPixelFormat::RGB32F)
    {
        throw std::invalid_argument("LightmapOutputWriter: unsupported lightmap pixel format");
    }

    const auto* values = std::get_if<std::vector<float>>(&pixels.rgb);
    if (!values || values->size() != byteCount)
    {
        throw std::invalid_argument("LightmapOutputWriter: RGB32F result has an invalid pixel buffer");
    }

    std::vector<uint8_t> result;
    result.reserve(byteCount);
    for (float value : *values)
    {
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("LightmapOutputWriter: RGB32F result contains a non-finite value");
        }
        const double clamped = std::clamp(static_cast<double>(value), 0.0, 1.0);
        result.push_back(static_cast<uint8_t>(std::lround(clamped * 255.0)));
    }
    return result;
}

std::shared_ptr<scene::Drawable> drawableFor(const std::shared_ptr<scene::Node>& node)
{
    if (!node || !node->drawable())
    {
        throw std::invalid_argument("LightmapOutputWriter: target must directly contain a Drawable");
    }
    auto drawable = node->drawable()->drawable();
    if (!drawable || !drawable->mesh())
    {
        throw std::invalid_argument("LightmapOutputWriter: target Drawable has no mesh");
    }
    return drawable;
}

struct MaterialTexture {
    Path path;
    std::string key;
};

std::vector<MaterialTexture> materialTexturePaths(
    const std::shared_ptr<scene::Drawable>& drawable,
    const Path& inputModelPath,
    bool includeAo)
{
    std::map<std::string, MaterialTexture> result;
    const auto add = [&](const std::shared_ptr<base::Texture>& texture) {
        if (!texture || texture->imageFilePath().empty())
        {
            return;
        }

        Path texturePath(texture->imageFilePath());
        if (texturePath.is_relative())
        {
            texturePath = inputModelPath.parent_path() / texturePath;
        }
        texturePath = absoluteNormalized(texturePath, "material texture");
        std::error_code error;
        if (!std::filesystem::is_regular_file(texturePath, error) || error)
        {
            throw std::runtime_error("LightmapOutputWriter: referenced material texture is missing: '" +
                texturePath.string() + "'");
        }
        const auto key = pathKey(texturePath);
        result.emplace(key, MaterialTexture{ texturePath, key });
    };

    for (uint32_t index = 0; index < drawable->submeshesCount(); ++index)
    {
        const auto& material = drawable->material(index);
        add(material.albedoTexture());
        add(material.metalnessTexture());
        add(material.roughnessTexture());
        add(material.normalTexture());
        if (includeAo)
        {
            add(material.aoTexture());
        }
        add(material.lightEmissionTexture());
    }

    std::vector<MaterialTexture> textures;
    textures.reserve(result.size());
    for (auto& [key, value] : result)
    {
        textures.push_back(std::move(value));
    }
    return textures;
}

void collectSceneTextureInputs(
    scene::Node* node,
    std::unordered_set<std::string>& protectedKeys,
    std::unordered_set<const scene::Node*>& visited)
{
    if (!node || !visited.insert(node).second)
    {
        return;
    }

    if (node->drawable())
    {
        auto drawable = node->drawable()->drawable();
        if (drawable)
        {
            for (const auto& texture : materialTexturePaths(drawable, {}, true))
            {
                protectedKeys.insert(texture.key);
            }
        }
    }
    if (node->environment() && !node->environment()->environmentImage().empty())
    {
        const auto environmentPath = absoluteNormalized(
            node->environment()->environmentImage(), "environment texture");
        std::error_code error;
        if (!std::filesystem::is_regular_file(environmentPath, error) || error)
        {
            throw std::runtime_error("LightmapOutputWriter: environment texture is missing: '" +
                environmentPath.string() + "'");
        }
        protectedKeys.insert(pathKey(environmentPath));
    }

    for (const auto& child : node->children())
    {
        if (child)
        {
            collectSceneTextureInputs(child.get(), protectedKeys, visited);
        }
    }
}

Path uniquePendingImagePath(const Path& finalPath)
{
    for (;;)
    {
        const auto suffix = temporaryCounter.fetch_add(1, std::memory_order_relaxed);
        auto filename = finalPath.stem().string() + ".pending-" + std::to_string(suffix) +
            finalPath.extension().string();
        auto candidate = finalPath.parent_path() / filename;
        if (!pathExistsIncludingSymlink(candidate))
        {
            return candidate;
        }
    }
}

Path uniqueStageDirectory(const Path& outputDirectory)
{
    for (;;)
    {
        const auto suffix = temporaryCounter.fetch_add(1, std::memory_order_relaxed);
        auto candidate = outputDirectory / (".bg2e-lightmap-stage-" + std::to_string(suffix));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error))
        {
            return candidate;
        }
        if (error && error != std::errc::file_exists)
        {
            throw std::runtime_error("LightmapOutputWriter: could not create staging directory '" +
                candidate.string() + "': " + error.message());
        }
    }
}

void removeNoThrow(const Path& path)
{
    std::error_code error;
    std::filesystem::remove(path, error);
}

void removeAllNoThrow(const Path& path)
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
}

}

struct LightmapOutputWriter::State {
    struct Sidecar {
        Path sourcePath;
        Path outputPath;
        std::string outputKey;
        std::string sourceKey;
    };

    struct TargetPlan {
        Target target;
        OutputPaths outputs;
        std::vector<Sidecar> sidecars;
        bool written = false;
    };

    Path outputDirectory;
    std::string extension;
    bool overwrite = false;
    bool preflightDone = false;
    std::vector<TargetPlan> targets;
    std::unordered_map<std::string, size_t> targetByIdentity;
    std::unordered_set<std::string> committedSidecars;
};

LightmapOutputWriter::LightmapOutputWriter(Path outputDirectory, ImageFormat format)
    : _state(std::make_unique<State>())
{
    _state->outputDirectory = absoluteNormalized(outputDirectory, "output directory");
    _state->extension = canonicalImageExtension(format);
    if (_state->extension.empty())
    {
        throw std::invalid_argument("LightmapOutputWriter: unsupported image format");
    }

    std::error_code error;
    const bool outputExists = std::filesystem::exists(_state->outputDirectory, error);
    if (error)
    {
        throw std::runtime_error("LightmapOutputWriter: could not inspect output directory '" +
            _state->outputDirectory.string() + "': " + error.message());
    }
    if (outputExists &&
        !std::filesystem::is_directory(_state->outputDirectory))
    {
        throw std::invalid_argument("LightmapOutputWriter: output path is not a directory: '" +
            _state->outputDirectory.string() + "'");
    }
}

LightmapOutputWriter::~LightmapOutputWriter() = default;
LightmapOutputWriter::LightmapOutputWriter(LightmapOutputWriter&&) noexcept = default;
LightmapOutputWriter& LightmapOutputWriter::operator=(LightmapOutputWriter&&) noexcept = default;

void LightmapOutputWriter::setOverwriteExisting(bool overwrite)
{
    if (!_state)
    {
        throw std::logic_error("LightmapOutputWriter: moved-from writer");
    }
    if (_state->preflightDone)
    {
        throw std::logic_error("LightmapOutputWriter: setOverwriteExisting must be called before preflight");
    }
    _state->overwrite = overwrite;
}

std::vector<LightmapOutputWriter::OutputPaths> LightmapOutputWriter::preflight(
    const std::vector<Target>& targets,
    bool writeModelCopies,
    const std::vector<Path>& protectedInputPaths)
{
    if (!_state)
    {
        throw std::logic_error("LightmapOutputWriter: moved-from writer");
    }
    if (_state->preflightDone)
    {
        throw std::logic_error("LightmapOutputWriter: preflight may only be called once");
    }
    if (targets.empty())
    {
        throw std::invalid_argument("LightmapOutputWriter: at least one target is required");
    }

    std::error_code error;
    const bool outputExists = std::filesystem::exists(_state->outputDirectory, error);
    if (error)
    {
        throw std::runtime_error("LightmapOutputWriter: could not inspect output directory '" +
            _state->outputDirectory.string() + "': " + error.message());
    }
    if (outputExists &&
        !std::filesystem::is_directory(_state->outputDirectory))
    {
        throw std::invalid_argument("LightmapOutputWriter: output path is not a directory: '" +
            _state->outputDirectory.string() + "'");
    }

    std::unordered_set<std::string> protectedKeys;
    const auto addProtectedInput = [&](const Path& path, const std::string& description) {
        auto absolutePath = absoluteNormalized(path, description);
        std::error_code statusError;
        if (!std::filesystem::exists(absolutePath, statusError) || statusError)
        {
            throw std::runtime_error("LightmapOutputWriter: protected input does not exist: '" +
                absolutePath.string() + "'");
        }
        protectedKeys.insert(pathKey(absolutePath));
    };
    for (const auto& path : protectedInputPaths)
    {
        addProtectedInput(path, "protected input");
    }

    if (!targets.front().node)
    {
        throw std::invalid_argument("LightmapOutputWriter: target node must not be null");
    }
    auto* assembledRoot = targets.front().node->sceneRoot();
    if (!assembledRoot)
    {
        throw std::invalid_argument("LightmapOutputWriter: target is not attached to an assembled scene");
    }
    for (const auto& target : targets)
    {
        if (!target.node || target.node->sceneRoot() != assembledRoot || !target.node->drawable())
        {
            throw std::invalid_argument(
                "LightmapOutputWriter: all targets must be attached Drawables in one assembled scene");
        }
    }
    std::unordered_set<const scene::Node*> visitedNodes;
    collectSceneTextureInputs(assembledRoot, protectedKeys, visitedNodes);

    std::vector<std::string> stems;
    stems.reserve(targets.size());
    std::unordered_set<std::string> identities;
    for (const auto& target : targets)
    {
        if (target.identity.empty() || !identities.insert(target.identity).second)
        {
            throw std::invalid_argument("LightmapOutputWriter: target identities must be nonempty and unique");
        }
        stems.push_back(safeStem(target.identity));
    }
    std::unordered_map<std::string, size_t> stemCounts;
    for (const auto& stem : stems)
    {
        ++stemCounts[stem];
    }
    for (size_t index = 0; index < stems.size(); ++index)
    {
        if (stemCounts[stems[index]] > 1)
        {
            stems[index] += "-" + stableHash(targets[index].identity);
        }
    }

    enum class ArtifactKind { FinalOutput, Sidecar };
    struct Reservation {
        ArtifactKind kind;
        std::string sourceKey;
    };
    std::unordered_map<std::string, Reservation> reservations;
    const auto reserveOutput = [&](const Path& path, ArtifactKind kind,
                                   const std::string& sourceKey, bool allowSameSourceSidecar) {
        const auto key = pathKey(path);
        if (protectedKeys.contains(key))
        {
            throw std::runtime_error("LightmapOutputWriter: output path aliases an input resource: '" +
                path.string() + "'");
        }
        if (!_state->overwrite && pathExistsIncludingSymlink(path))
        {
            throw std::runtime_error("LightmapOutputWriter: output path already exists: '" +
                path.string() + "'");
        }
        const auto existing = reservations.find(key);
        if (existing != reservations.end())
        {
            if (allowSameSourceSidecar && kind == ArtifactKind::Sidecar &&
                existing->second.kind == ArtifactKind::Sidecar &&
                existing->second.sourceKey == sourceKey)
            {
                return key;
            }
            throw std::runtime_error("LightmapOutputWriter: output paths collide at '" +
                path.string() + "'");
        }
        reservations.emplace(key, Reservation{ kind, sourceKey });
        return key;
    };

    std::vector<State::TargetPlan> plannedTargets;
    plannedTargets.reserve(targets.size());
    for (size_t index = 0; index < targets.size(); ++index)
    {
        const auto& target = targets[index];
        const auto modelInput = absoluteNormalized(target.inputModelPath, "target model");
        addProtectedInput(modelInput, "target model");
        if (!target.node || !target.node->drawable())
        {
            throw std::invalid_argument("LightmapOutputWriter: target must directly contain a Drawable");
        }
        auto drawable = drawableFor(target.node);
        const auto textures = materialTexturePaths(drawable, modelInput, true);
        for (const auto& texture : textures)
        {
            protectedKeys.insert(texture.key);
        }

        OutputPaths outputs;
        outputs.identity = target.identity;
        outputs.imagePath = _state->outputDirectory / (stems[index] + _state->extension);
        reserveOutput(outputs.imagePath, ArtifactKind::FinalOutput, {}, false);
        if (writeModelCopies)
        {
            auto modelPath = _state->outputDirectory / (stems[index] + ".bg2");
            reserveOutput(modelPath, ArtifactKind::FinalOutput, {}, false);
            outputs.modelPath = std::move(modelPath);
        }

        State::TargetPlan targetPlan;
        targetPlan.target = target;
        targetPlan.outputs = outputs;
        if (writeModelCopies)
        {
            // The output lightmap replaces any existing AO texture, so that
            // source remains protected but is not copied as an output sidecar.
            const auto otherTextures = materialTexturePaths(drawable, modelInput, false);
            for (const auto& texture : otherTextures)
            {
                const auto sidecarPath = _state->outputDirectory / texture.path.filename();
                const auto key = reserveOutput(sidecarPath, ArtifactKind::Sidecar,
                                              texture.key, true);
                targetPlan.sidecars.push_back({ texture.path, sidecarPath, key, texture.key });
            }
        }

        plannedTargets.push_back(std::move(targetPlan));
    }

    // Textures are protected input resources too. Recheck all reserved names
    // after collecting them, so an output cannot replace its own source.
    for (const auto& [key, reservation] : reservations)
    {
        (void)reservation;
        if (protectedKeys.contains(key))
        {
            throw std::runtime_error("LightmapOutputWriter: output path aliases an input resource: '" + key + "'");
        }
    }

    _state->targets = std::move(plannedTargets);
    _state->targetByIdentity.clear();
    std::vector<OutputPaths> result;
    result.reserve(_state->targets.size());
    for (size_t index = 0; index < _state->targets.size(); ++index)
    {
        _state->targetByIdentity.emplace(_state->targets[index].target.identity, index);
        result.push_back(_state->targets[index].outputs);
    }
    _state->preflightDone = true;
    return result;
}

void LightmapOutputWriter::write(const render::LightmapPixels& pixels, std::string_view targetIdentity)
{
    if (!_state || !_state->preflightDone)
    {
        throw std::logic_error("LightmapOutputWriter: preflight must succeed before writing");
    }
    const auto targetPosition = _state->targetByIdentity.find(std::string(targetIdentity));
    if (targetPosition == _state->targetByIdentity.end())
    {
        throw std::invalid_argument("LightmapOutputWriter: target identity was not preflighted");
    }
    auto& targetPlan = _state->targets[targetPosition->second];
    if (targetPlan.written)
    {
        throw std::logic_error("LightmapOutputWriter: target output has already been written");
    }

    const auto rgb = rgb8Pixels(pixels);
    std::error_code directoryError;
    std::filesystem::create_directories(_state->outputDirectory, directoryError);
    if (directoryError)
    {
        throw std::runtime_error("LightmapOutputWriter: could not create output directory '" +
            _state->outputDirectory.string() + "': " + directoryError.message());
    }

    const auto pendingImage = uniquePendingImagePath(targetPlan.outputs.imagePath);
    const auto replaceWith = [&](const Path& staged, const Path& output) {
        if (pathExistsIncludingSymlink(output))
        {
            if (!_state->overwrite)
            {
                throw std::runtime_error("LightmapOutputWriter: output path appeared after preflight: '" +
                    output.string() + "'");
            }
            std::error_code removeError;
            if (!std::filesystem::remove(output, removeError) || removeError)
            {
                throw std::runtime_error("LightmapOutputWriter: could not overwrite existing output '" +
                    output.string() + "'");
            }
        }
        std::filesystem::rename(staged, output);
    };
    Path stageDirectory;
    std::vector<Path> committedThisWrite;
    std::vector<std::string> sidecarsCommittedThisWrite;
    try
    {
        saveImage(pendingImage, rgb.data(), pixels.width, pixels.height, 3);

        if (targetPlan.outputs.modelPath)
        {
            stageDirectory = uniqueStageDirectory(_state->outputDirectory);
            const auto stagedImage = stageDirectory / targetPlan.outputs.imagePath.filename();
            std::filesystem::copy_file(pendingImage, stagedImage,
                                       std::filesystem::copy_options::none);

            auto sourceDrawable = drawableFor(targetPlan.target.node);
            auto drawableBaseCopy = sourceDrawable->clone();
            auto drawableCopy = std::dynamic_pointer_cast<scene::Drawable>(drawableBaseCopy);
            if (!drawableCopy)
            {
                throw std::runtime_error("LightmapOutputWriter: could not clone target Drawable");
            }

            auto lightmapTexture = std::make_shared<base::Texture>(stagedImage);
            for (uint32_t submesh = 0; submesh < drawableCopy->submeshesCount(); ++submesh)
            {
                auto& material = drawableCopy->material(submesh);
                material.setAoTexture(lightmapTexture);
                material.setAoScale(glm::vec2(1.0f, 1.0f));
                material.setAoUVSet(1);
                material.setAoChannel(0);
            }

            const auto stagedModel = stageDirectory / targetPlan.outputs.modelPath->filename();
            storeDrawableBg2(stagedModel, drawableCopy.get());

            for (const auto& sidecar : targetPlan.sidecars)
            {
                const auto stagedSidecar = stageDirectory / sidecar.sourcePath.filename();
                if (!std::filesystem::is_regular_file(stagedSidecar))
                {
                    throw std::runtime_error("LightmapOutputWriter: expected staged texture was not written: '" +
                        stagedSidecar.string() + "'");
                }
                if (_state->committedSidecars.contains(sidecar.outputKey))
                {
                    removeNoThrow(stagedSidecar);
                    continue;
                }
                replaceWith(stagedSidecar, sidecar.outputPath);
                committedThisWrite.push_back(sidecar.outputPath);
                sidecarsCommittedThisWrite.push_back(sidecar.outputKey);
            }

            replaceWith(stagedModel, *targetPlan.outputs.modelPath);
            committedThisWrite.push_back(*targetPlan.outputs.modelPath);

            replaceWith(pendingImage, targetPlan.outputs.imagePath);
            committedThisWrite.push_back(targetPlan.outputs.imagePath);

            for (const auto& key : sidecarsCommittedThisWrite)
            {
                _state->committedSidecars.insert(key);
            }
        }
        else
        {
            replaceWith(pendingImage, targetPlan.outputs.imagePath);
            committedThisWrite.push_back(targetPlan.outputs.imagePath);
        }

        targetPlan.written = true;
    }
    catch (...)
    {
        for (auto path = committedThisWrite.rbegin(); path != committedThisWrite.rend(); ++path)
        {
            removeNoThrow(*path);
        }
        for (const auto& key : sidecarsCommittedThisWrite)
        {
            _state->committedSidecars.erase(key);
        }
        removeNoThrow(pendingImage);
        if (!stageDirectory.empty())
        {
            removeAllNoThrow(stageDirectory);
        }
        throw;
    }

    if (!stageDirectory.empty())
    {
        removeAllNoThrow(stageDirectory);
    }
}

const Path& LightmapOutputWriter::outputDirectory() const
{
    if (!_state)
    {
        throw std::logic_error("LightmapOutputWriter: moved-from writer");
    }
    return _state->outputDirectory;
}

}
