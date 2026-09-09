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

#include <bg2e/db/scene_save_dialog.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/MessageBox.hpp>

namespace bg2e::db {

std::filesystem::path confirmSceneSavePath(const std::filesystem::path& selectedPath,
    SceneSavePathSource source, SceneArtifactType type, SceneOverwritePolicy overwritePolicy)
{
    if (selectedPath.empty()) return {};
    const auto target = inspectSceneSaveTarget(selectedPath, type);

    // When the final path is unchanged from the accepted native save-file
    // dialog, assume that dialog already checked the path and confirmed any
    // overwrite. Do not add another existence-based prompt in that case: it
    // would ask twice about the same file. A transformed path was not checked
    // by the dialog, and a direct save had no dialog, so both need our check
    // unless the caller explicitly authorizes overwriting with Allow.
    const bool confirmedByDialog = source == SceneSavePathSource::NativeSaveDialog && !target.pathChanged;
    if (target.fileExists && !confirmedByDialog && overwritePolicy != SceneOverwritePolicy::Allow)
    {
        using bg2e::app::MessageBox;
        if (MessageBox::showWarning("Overwrite file",
            "The following file already exists. Do you want to overwrite it?\n\n" + target.path.string(),
            {
                { .code = 0, .label = "Cancel", .key = MessageBox::Esc },
                { .code = 1, .label = "Overwrite" }
            }) != 1) return {};
    }
    return target.path;
}

std::filesystem::path getSceneSavePath(SceneArtifactType type)
{
    bg2e::app::FileDialog dialog;
    dialog.setFilters({ { "JSON scene or prefab", "json" } });
    return confirmSceneSavePath(dialog.saveFile(), SceneSavePathSource::NativeSaveDialog, type);
}

}
