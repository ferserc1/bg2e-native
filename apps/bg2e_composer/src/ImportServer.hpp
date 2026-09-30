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
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

namespace httplib {
struct Request;
struct Response;
}

// Localhost HTTP server facade exposing POST /import and GET /status.
// httplib is hidden behind a pImpl and used only inside ImportServer.cpp.
// The HTTP worker thread never touches the scene graph: it enqueues
// one ImportRequest and blocks until Composer restores its UI.
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

    // Main thread: take the admitted request without freeing the occupied slot.
    bool takePending(ImportRequest& request);

    // Main thread: deliver the outcome; wakes the blocked HTTP handler.
    void fulfil(uint64_t requestId, bool ok, const std::string& message);

    bool busy() const;

private:
    // Runs on the HTTP worker thread: parse + validate JSON, enqueue the
    // request and block until fulfil() or service stop.
    void handleImport(const httplib::Request& req, httplib::Response& res);

    struct Impl;
    std::unique_ptr<Impl> _impl;

    mutable std::mutex _mutex;
    std::condition_variable _cv;
    struct Slot {
        ImportRequest request;
        bool taken = false;
        bool done = false;
        bool ok = false;
        std::string message;
    };
    std::shared_ptr<Slot> _slot;
    bool _accepting = false;
    uint64_t _nextId = 1;
};
