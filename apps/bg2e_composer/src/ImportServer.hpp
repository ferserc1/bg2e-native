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

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace httplib {
struct Request;
struct Response;
}

// Localhost HTTP server facade exposing POST /import and GET /status.
// httplib is hidden behind a pImpl and used only inside ImportServer.cpp.
// The HTTP worker thread never touches the scene graph: it enqueues
// ImportRequest items and blocks until the main thread fulfils them.
class ImportServer {
public:
    ImportServer();
    ~ImportServer();            // calls stop()

    // Binds synchronously (error known before returning) and launches the
    // listen thread. Returns "" on success, or a human-readable error
    // (invalid port / port in use) for the UI alert.
    std::string start(uint32_t port);
    void stop();                // stop() + join; safe to call when stopped
    bool isRunning() const;
    uint32_t port() const;

    // Main thread: drain all pending requests (queue is emptied).
    std::vector<ImportRequest> popPending();

    // Main thread: deliver the outcome; wakes the blocked HTTP handler.
    void fulfil(uint64_t requestId, bool ok, const std::string& message);

    size_t pendingCount() const;

private:
    // Runs on the HTTP worker thread: parse + validate JSON, enqueue the
    // request and block on the per-request slot until fulfil() or timeout.
    void handleImport(const httplib::Request& req, httplib::Response& res);

    struct Impl;
    std::unique_ptr<Impl> _impl;

    mutable std::mutex _mutex;
    std::deque<ImportRequest> _queue;
    struct Slot {
        bool done = false;
        bool ok = false;
        std::string message;
    };
    std::unordered_map<uint64_t, std::shared_ptr<Slot>> _results;
    uint64_t _nextId = 1;
};
