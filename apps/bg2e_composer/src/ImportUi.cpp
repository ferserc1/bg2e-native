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

#include "EditorUi.hpp"

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

namespace bg2e_composer {

void EditorUi::chooseImport(const std::shared_ptr<AppDelegate>& delegate)
{
    const auto path = bg2e::app::FileDialog::getOpenFilePath(
        {{"Models and scene prefabs", "bg2,vwglb,obj,gltf,glb,json,bg2scene,bg2e,vitscnj"}});
    if (!path.empty())
    {
        const float scales[]{1, 0.01f, 0.001f, 0.0254f, 0.3048f};
        delegate->importDocument(path, scales[_importUnits], _importZUp);
    }
}

void EditorUi::drawImportSettings(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    if (_importPort.empty())
    {
        _importPort = std::to_string(delegate->importPort());
    }
    const bool running = delegate->importServiceRunning();
    ui::Value::text("Port", _importPort, 6, false, running);
    bool enabled = running;
    if (ui::Button::checkBox("Service enabled", &enabled))
    {
        delegate->setImportService(enabled, _importPort);
    }
    ui::Text::text(delegate->importStatus());
    ui::Text::separator("Local import conversion");
    ui::Value::comboBox("Source units", {"m", "cm", "mm", "in", "ft"}, _importUnits);
    ui::Button::checkBox("Source uses Z up", &_importZUp);
    ui::Text::text("HTTP requests specify their own units and coordinateSystem.");
}

void EditorUi::drawImportProgress(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    ui::Text::text("Waiting jobs: " + std::to_string(delegate->imports().queued()));
    ui::Text::text(delegate->importStatus());
    if (auto job = delegate->imports().active(); job && job->cancelled.load())
    {
        ui::Text::text("Waiting for the cancelled reader to finish.");
    }
    if (ui::Button::button("Cancel imports"))
    {
        delegate->cancelImports();
    }
    if (ui::Button::button("Queue another import..."))
    {
        chooseImport(delegate);
    }
    if (delegate->importServiceRunning() && ui::Button::button("Stop import service"))
    {
        delegate->setImportService(false, {});
    }
}

}
