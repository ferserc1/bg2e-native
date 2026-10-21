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

#include "SceneImporter.hpp"

#include <bg2e/db/scene.hpp>
#include <bg2e/db/scene_path.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace bg2e_composer {

SceneImporter::~SceneImporter()
{
    shutdown();
}

bool SceneImporter::enqueue(std::shared_ptr<Job> job)
{
    if (!job || _queue.size() >= 16)
    {
        return false;
    }
    _queue.push_back(std::move(job));
    return true;
}

void SceneImporter::startNext(bg2e::app::MainLoop* loop)
{
    if (_active || _queue.empty())
    {
        return;
    }
    _active = _queue.front();
    _queue.pop_front();
    auto job = _active;
    try
    {
        _worker = std::thread([job, loop]
        {
            try
            {
                if (!job->cancelled.load())
                {
                    auto path = std::filesystem::absolute(job->request.filePath).lexically_normal();
                    if (std::filesystem::is_directory(path))
                    {
                        path = bg2e::db::resolveSceneLoadPath(path);
                    }
                    if (!std::filesystem::is_regular_file(path))
                    {
                        throw std::invalid_argument("Import source is not a regular file");
                    }
                    job->request.filePath = path;
                    auto extension = path.extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
                    {
                        return static_cast<char>(std::tolower(c));
                    });
                    job->loader.setMessage("Reading " + job->request.fileName);
                    job->loader.setProgress(0.1f);
                    if (extension == ".gltf" || extension == ".glb")
                    {
                        job->gltf = bg2e::db::readGltf(path);
                    }
                    else if (extension == ".json" || extension == ".bg2scene" || extension == ".bg2e" || extension == ".vitscnj")
                    {
                        job->schema = bg2e::db::readSceneDocument(path);
                    }
                    else if (extension == ".bg2" || extension == ".vwglb")
                    {
                        job->bg2.reset(bg2e::db::loadMeshBg2(path));
                    }
                    else if (extension == ".obj")
                    {
                        job->obj = bg2e::db::readObj(path);
                    }
                    else
                    {
                        throw std::invalid_argument("Unsupported import format");
                    }
                    job->loader.setMessage("Waiting to realize import resources");
                    job->loader.setProgress(0.4f);
                }
            }
            catch (const std::exception& error)
            {
                job->error = error.what();
            }
            catch (...)
            {
                job->error = "Unknown import parser error";
            }
            job->done.store(true, std::memory_order_release);
            if (loop)
            {
                loop->requestFrame();
            }
        });
    }
    catch (const std::exception& error)
    {
        job->error = error.what();
        job->done.store(true, std::memory_order_release);
    }
}

std::shared_ptr<SceneImporter::Job> SceneImporter::takeCompleted()
{
    if (!_active || !_active->done.load(std::memory_order_acquire))
    {
        return {};
    }
    if (_worker.joinable())
    {
        _worker.join();
    }
    return std::exchange(_active, {});
}

std::vector<std::shared_ptr<SceneImporter::Job>> SceneImporter::cancel(bool remoteOnly)
{
    std::vector<std::shared_ptr<Job>> cancelled;
    auto cancelJob = [&](const std::shared_ptr<Job>& job)
    {
        if (job && (!remoteOnly || job->request.id != 0) && !job->cancelled.exchange(true))
        {
            cancelled.push_back(job);
        }
    };
    cancelJob(_active);
    for (const auto& job : _queue)
    {
        cancelJob(job);
    }
    std::erase_if(_queue, [](const auto& job)
    {
        return job->cancelled.load();
    });
    return cancelled;
}

void SceneImporter::shutdown()
{
    cancel();
    if (_worker.joinable())
    {
        _worker.join();
    }
    _active.reset();
}

}
