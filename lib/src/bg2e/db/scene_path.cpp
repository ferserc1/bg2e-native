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

#include <bg2e/db/scene_path.hpp>

#include <stdexcept>
#include <system_error>

namespace bg2e::db {
namespace {
namespace fs = std::filesystem;

bool validName(const fs::path& path)
{
    const auto name = path.filename();
    return !path.empty() && !name.empty() && name != "." && name != "..";
}

fs::path absoluteSelection(const fs::path& path)
{
    // Removing '.' is safe, but collapsing '..' could change the destination
    // when an earlier component is a symbolic link.
    fs::path result;
    for (const auto& component : fs::absolute(path))
        if (component != ".") result /= component;
    return result;
}

void checkExactSpelling(const fs::path& path)
{
    for (const auto& entry : fs::directory_iterator(path.parent_path()))
        if (entry.path().filename() == path.filename()) return;
    throw std::runtime_error("The scene destination spelling must match the scene name: " + path.string());
}

fs::file_status linkStatus(const fs::path& path)
{
    std::error_code error;
    const auto status = fs::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory)
        throw fs::filesystem_error("Cannot inspect scene destination", path, error);
    return status;
}
}

fs::path resolveSceneLoadPath(const fs::path& selectedPath)
{
    if (selectedPath.empty())
        throw std::invalid_argument("A scene path is required.");

    auto result = absoluteSelection(selectedPath);
    if (fs::is_directory(result))
    {
        // Canonicalize directories to handle trailing separators, '.' and '..'.
        result = fs::canonical(result);
        const auto name = result.filename();
        if (!validName(name))
            throw std::invalid_argument("A named scene directory is required.");
        result /= name.string() + ".json";
    }
    if (result.extension() != ".json")
        throw std::invalid_argument("The scene file must have a .json extension: " + result.string());
    if (!fs::is_regular_file(result))
        throw std::runtime_error("Scene file not found or not a regular file: " + result.string());
    return result;
}

bool isValidScenePath(const fs::path& jsonPath, SceneArtifactType type)
{
    if (!validName(jsonPath) || jsonPath.extension() != ".json" ||
        !validName(jsonPath.stem())) return false;
    if (type == SceneArtifactType::Prefab) return true;
    if (type != SceneArtifactType::Scene) return false;
    try
    {
        return absoluteSelection(jsonPath).parent_path().filename() == jsonPath.stem();
    }
    catch (const fs::filesystem_error&)
    {
        return false;
    }
}

fs::path resolveSceneSavePath(const fs::path& selectedPath, SceneArtifactType type)
{
    if (!validName(selectedPath))
        throw std::invalid_argument("A scene filename is required.");
    if (type != SceneArtifactType::Scene && type != SceneArtifactType::Prefab)
        throw std::invalid_argument("Unknown scene artifact type.");

    auto result = absoluteSelection(selectedPath);
    if (result.extension() != ".json") result += ".json";
    const auto name = result.stem();
    if (!validName(name))
        throw std::invalid_argument("A scene filename is required.");
    if (type == SceneArtifactType::Scene && result.parent_path().filename() != name)
        result = result.parent_path() / name / result.filename();
    return result;
}

SceneSaveTarget inspectSceneSaveTarget(const fs::path& selectedPath, SceneArtifactType type)
{
    SceneSaveTarget target;
    target.path = resolveSceneSavePath(selectedPath, type);
    // Absolute conversion alone is not a change to the dialog's destination.
    target.pathChanged = target.path != absoluteSelection(selectedPath);

    auto directory = target.path.parent_path();
    while (!directory.empty())
    {
        const auto status = linkStatus(directory);
        if (fs::exists(status))
        {
            if (!fs::is_directory(directory))
                throw std::runtime_error("The scene directory is not a directory: " + directory.string());
            break;
        }
        const auto parent = directory.parent_path();
        if (parent == directory) break;
        directory = parent;
    }

    // On case-insensitive filesystems, an existing differently cased directory
    // must not silently satisfy the exact scene-name requirement.
    const auto parent = target.path.parent_path();
    if (type == SceneArtifactType::Scene && fs::exists(parent) && parent != parent.root_path())
        checkExactSpelling(parent);

    const auto status = linkStatus(target.path);
    if (fs::is_symlink(status))
        throw std::runtime_error("The scene destination must not be a symbolic link: " + target.path.string());
    target.fileExists = fs::exists(status);
    if (target.fileExists && !fs::is_regular_file(status))
        throw std::runtime_error("The scene destination is not a regular file: " + target.path.string());
    if (target.fileExists && type == SceneArtifactType::Scene)
        checkExactSpelling(target.path);
    return target;
}

}
