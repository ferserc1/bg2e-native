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

#include <bg2e/db/scene_gltf.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/ui/Loader.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <atomic>
#include <deque>
#include <thread>
#include <vector>

namespace bg2e::draw {
class Renderer;
}

namespace bg2e_composer {

// App-owned single-worker queue. Worker jobs own only CPU data; scene/renderer
// identities are interpreted exclusively by AppDelegate on the main thread.
class SceneImporter {
  public:
    struct Job {
        ImportRequest request;
        uint64_t token = 0;
        uint64_t documentGeneration = 0;
        uint64_t rendererGeneration = 0;
        const bg2e::draw::Renderer* rendererIdentity = nullptr; // compare only; never dereference
        std::weak_ptr<bg2e::scene::Node> parent;
        std::weak_ptr<bg2e::scene::Node> previous;
        bool replacing = false;
        std::string key;
        bg2e::ui::Loader loader;
        std::atomic<bool> cancelled{false};
        std::atomic<bool> done{false};
        std::shared_ptr<bg2e::db::GltfDocument> gltf;
        std::shared_ptr<bg2e::db::Bg2Mesh> bg2;
        std::shared_ptr<bg2e::db::ObjMesh> obj;
        std::shared_ptr<bg2e::json::JsonNode> schema;
        std::string error;
    };

    ~SceneImporter();

    bool enqueue(std::shared_ptr<Job> job);
    void startNext(bg2e::app::MainLoop* loop);
    std::shared_ptr<Job> takeCompleted();
    std::vector<std::shared_ptr<Job>> cancel(bool remoteOnly = false);
    void shutdown();

    std::shared_ptr<Job> active() const
    {
        return _active;
    }
    size_t queued() const
    {
        return _queue.size();
    }
    bool busy() const
    {
        return (_active && !_active->cancelled.load()) || !_queue.empty();
    }

  private:
    std::deque<std::shared_ptr<Job>> _queue;
    std::shared_ptr<Job> _active;
    std::thread _worker;
};

}
