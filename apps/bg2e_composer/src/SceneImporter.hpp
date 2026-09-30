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

#include "ImportRequest.hpp"

#include <bg2e/scene/Node.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class StageScene;
class ImportServer;
class AppDelegate;

class SceneImporter {
public:
    SceneImporter(StageScene * stage, AppDelegate * appDelegate);

    // Called once per frame from AppDelegate::update() on the main thread.
    void processQueue(ImportServer& server);

    // Drops all tracked entries when the scene changes or the app shuts down.
    void clear();

private:
    struct ImportEntry {
        std::weak_ptr<bg2e::scene::Node> node;
        std::string identifier;
    };

    StageScene * _stage;
    AppDelegate * _appDelegate;
    std::unordered_map<std::string, ImportEntry> _table;

    static std::string tableKey(const std::filesystem::path& path);
    void sweep();
    void processOne(ImportServer& server, const ImportRequest& request);
};
