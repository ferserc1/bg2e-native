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

#include <bg2e/ui/Workspace.hpp>

#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace bg2e::ui;
namespace fs = std::filesystem;

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::string readFile(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}

int main(int argc, char** argv)
{
    require(argc == 2 && fs::is_directory(argv[1]), "Supply an empty temporary directory");
    const fs::path directory = argv[1];
    require(fs::is_empty(directory), "The temporary directory must be empty");

    const auto scenePath = directory / "composer-scene-tools.toolbar";
    const auto volumePath = directory / "composer-volume-tools.toolbar";
    const std::string legacySettings =
        "version=1\npresentation=0\niconSize=24\nleft=open,@separator,obsolete,save\nright=\nend\n";
    {
        std::ofstream file(scenePath, std::ios::binary);
        file << legacySettings;
        file.close();
        require(bool(file), "Write legacy settings");
    }

    Workspace scene;
    Workspace volume;
    WorkspaceCollection workspaces;
    require(workspaces.registerWorkspace(Workspace0, scene), "Register scene");
    require(workspaces.registerWorkspace(Workspace1, volume), "Register volume");
    workspaces.registerActions({
        {.id = "open", .execute = [] {}, .workspaceMask = Workspace0},
        {.id = "save", .execute = [] {}}
    });

    Toolbar sceneToolbar;
    workspaces.configureToolbar(Workspace0, sceneToolbar, {{}, {"open", "save"}}, scenePath);
    require(sceneToolbar.configuration().iconSize == 24.0f, "Load legacy appearance");

    Toolbar volumeToolbar;
    bool duplicateRejected = false;
    try
    {
        workspaces.configureToolbar(Workspace1, volumeToolbar, {{"save"}, {}},
            directory / "." / scenePath.filename());
    }
    catch (const std::invalid_argument&)
    {
        duplicateRejected = true;
    }
    require(duplicateRejected, "Reject alias of an assigned preferences file");

    volumeToolbar.setIconSize(32.0f);
    workspaces.configureToolbar(Workspace1, volumeToolbar, {{"save"}, {}}, volumePath);
    const auto volumeSettings = readFile(volumePath);
    require(volumeSettings.starts_with("version=1\n") &&
        volumeSettings.find("iconSize=32\n") != std::string::npos, "Save independent volume appearance");
    require(readFile(scenePath) == legacySettings, "Preserve the existing scene file");

    require(workspaces.activate(Workspace0) && workspaces.activate(Workspace1), "Switch workspaces");
    require(sceneToolbar.configuration().iconSize == 24.0f &&
        volumeToolbar.configuration().iconSize == 32.0f, "Keep independent in-memory settings");
    require(readFile(scenePath) == legacySettings &&
        readFile(volumePath) == volumeSettings, "Switching leaves both files intact");

    Toolbar standaloneToolbar;
    standaloneToolbar.registerActions({{.id = "open", .execute = [] {}}, {.id = "save", .execute = [] {}}});
    standaloneToolbar.enableCustomization(scenePath);
    require(standaloneToolbar.configuration().iconSize == 24.0f, "Standalone file still loads");
}
