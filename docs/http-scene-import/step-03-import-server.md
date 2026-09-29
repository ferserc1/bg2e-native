# Step 03 — ImportServer (cpp-httplib integration + request queue)

## Goal

HTTP layer of the feature: a localhost-only server exposing
`POST /import` and `GET /status`, a thread-safe import queue, and synchronous
per-request completion. The worker thread never touches the scene graph.

## Vendored library facts (verified against `apps/third_party/cpp-httplib/httplib.h` and upstream docs)

- Version: **0.58.0** (`CPPHTTPLIB_VERSION "0.58.0"`).
- Header-only; include it from **one dedicated `.cpp`** (compile-time hygiene).
- Plain HTTP by default; `CPPHTTPLIB_OPENSSL_SUPPORT` must **not** be defined.
- `listen(host, port)` blocks → run on a dedicated `std::thread`; `stop()` is
  `noexcept` and thread-safe; `wait_until_ready()` confirms startup.
- `bind_to_port(host, port)` returns `bool` → synchronous port-occupied
  detection before `listen_after_bind()`.
- Default socket opts use `SO_REUSEPORT` on POSIX → override with
  `set_socket_options()` (`SO_REUSEADDR` on POSIX, `SO_EXCLUSIVEADDRUSE` on
  Windows) so binding an occupied port fails.
- Single worker: `svr.new_task_queue = [] { return new httplib::ThreadPool(1, 1); };`
  serializes handlers (orderly queue, no concurrent JSON handling).
- JSON parsing: engine's own `bg2e::json::JsonParser` (parses from
  `std::string`) — no extra dependency.

## Files

### Modify: `apps/bg2e_composer/CMakeLists.txt` (approved)

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME bg2e_composer)
bundle_app(TARGET_NAME ${APP_TARGET_NAME})

target_include_directories(${APP_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/third_party/cpp-httplib
)
```

### Create: `apps/bg2e_composer/src/ImportRequest.hpp`

```cpp
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

// Coordinate system of the source file. bg2e/glTF native is y_up.
enum class ImportCoordinateSystem {
    YUp,    // no conversion (glTF native)
    ZUp     // wrapper gets -90 deg rotation on X
};

struct ImportRequest {
    uint64_t id = 0;                        // assigned by ImportServer
    std::string fileName;                   // informational / fallback node name
    std::filesystem::path filePath;         // absolute local path (validated)
    float unitsScale = 1.0f;                // meters per source unit
    ImportCoordinateSystem coordinateSystem = ImportCoordinateSystem::YUp;
};
```

### Create: `apps/bg2e_composer/src/ImportServer.hpp`

httplib stays out of the public interface (pImpl):

```cpp
#pragma once

#include "ImportRequest.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

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
    struct Impl;
    std::unique_ptr<Impl> _impl;

    std::mutex _mutex;
    std::deque<ImportRequest> _queue;
    struct Slot {
        bool done = false;
        bool ok = false;
        std::string message;
    };
    std::unordered_map<uint64_t, std::shared_ptr<Slot>> _results;
    uint64_t _nextId = 1;
};
```

### Create: `apps/bg2e_composer/src/ImportServer.cpp`

Single TU including httplib. Key fragments:

```cpp
#include "ImportServer.hpp"

#include <httplib.h>                      // sole includer of httplib.h
#include <bg2e/json/JsonParser.hpp>       // engine JSON parser

#include <chrono>
#include <condition_variable>

using namespace std::chrono_literals;
static constexpr auto kRequestTimeout = 60s;

struct ImportServer::Impl {
    httplib::Server server;
    std::thread listenThread;
    uint32_t boundPort = 0;
    std::condition_variable cv;           // signaled by fulfil()
    std::mutex cvMutex;                   // guards Slots (paired with _mutex use)
};
```

**start():** synchronous bind + background listen.

```cpp
std::string ImportServer::start(uint32_t port)
{
    if (_impl->listenThread.joinable()) return "Service already running";
    if (port < 1024 || port > 49151) return "Port out of valid range (1024-49151)";

    auto& svr = _impl->server;

    // Make an occupied port fail the bind (default SO_REUSEPORT would share it)
    svr.set_socket_options([](httplib::socket_t sock) {
    #ifdef _WIN32
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
    #else
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_REUSEADDR, 1);
    #endif
    });

    // Serialize all request handling on one worker thread
    svr.new_task_queue = [] { return new httplib::ThreadPool(1, 1); };

    svr.Get("/status", [this](const httplib::Request&, httplib::Response& res) {
        std::string body = std::string("{\"running\":true,\"port\":") +
            std::to_string(_impl->boundPort) +
            ",\"queued\":" + std::to_string(pendingCount()) + "}";
        res.set_content(body, "application/json");
    });

    svr.Post("/import", [this](const httplib::Request& req, httplib::Response& res) {
        handleImport(req, res);            // see below
    });

    if (!svr.bind_to_port("127.0.0.1", static_cast<int>(port)))
    {
        return "Port " + std::to_string(port) + " is already in use";
    }
    _impl->boundPort = port;

    _impl->listenThread = std::thread([&svr]() {
        svr.listen_after_bind();           // blocks until stop()
    });
    svr.wait_until_ready();
    return "";
}
```

**stop():**

```cpp
void ImportServer::stop()
{
    if (!_impl->listenThread.joinable()) return;
    _impl->server.stop();                  // noexcept, wakes the accept loop
    _impl->listenThread.join();
    _impl->server = httplib::Server{};     // fresh instance for the next start()
    _impl->boundPort = 0;

    // Fail every request still waiting so no handler outlives the server
    std::lock_guard lock(_mutex);
    for (auto& [id, slot] : _results)
    {
        slot->done = true; slot->ok = false; slot->message = "Service stopped";
    }
    _queue.clear();
    _impl->cv.notify_all();
}
```

**handleImport():** parse, validate, enqueue, block until fulfilled.

```cpp
void handleImport(const httplib::Request& req, httplib::Response& res)
{
    if (req.get_header_value("Content-Type").find("application/json") != 0)
        return fail(res, 415, "Content-Type must be application/json");

    auto root = bg2e::json::JsonParser(req.body).parse();
    if (!root) return fail(res, 400, "Malformed JSON body");

    ImportRequest r;
    std::string units, coordSystem;
    // extract fileName / filePath / units / coordinateSystem via JsonNode
    // accessors; missing filePath -> 400; unknown units/coords -> 400
    // units: "m"=1, "cm"=0.01, "mm"=0.001, "in"=0.0254, "ft"=0.3048
    // coordinateSystem: "y_up" (default), "z_up"

    if (!std::filesystem::exists(r.filePath))
        return fail(res, 404, "File not found: " + r.filePath.string());
    auto ext = r.filePath.extension().string();
    if (ext != ".gltf" && ext != ".glb")
        return fail(res, 400, "Only .gltf/.glb files are supported");

    auto slot = std::make_shared<Slot>();
    {
        std::lock_guard lock(_mutex);
        r.id = _nextId++;
        _results[r.id] = slot;
        _queue.push_back(std::move(r));
    }

    // Block the HTTP worker until the main thread processes the request
    std::unique_lock lk(_impl->cvMutex);
    bool done = _impl->cv.wait_for(lk, kRequestTimeout, [&]{ return slot->done; });
    lk.unlock();

    {
        std::lock_guard lock(_mutex);
        _results.erase(r.id);              // cleanup either way
    }

    if (!done) return fail(res, 504, "Import timed out waiting for the main thread");
    if (!slot->ok) return fail(res, 500, slot->message);

    res.status = 200;
    res.set_content("{\"status\":\"ok\",\"message\":\"" + slot->message + "\"}",
                    "application/json");
}
```

**fulfil()** (called from the main thread by `SceneImporter`):

```cpp
void ImportServer::fulfil(uint64_t id, bool ok, const std::string& message)
{
    {
        std::lock_guard lock(_mutex);
        auto it = _results.find(id);
        if (it == _results.end()) return;   // timed out already
        it->second->done = true;
        it->second->ok = ok;
        it->second->message = message;
    }
    _impl->cv.notify_all();
}
```

## Integration points

- Owned by `AppDelegate` (step 06); constructed early, started from
  `initWorkspace()` if `ImportSettings::serviceEnabled()`, stopped in
  `cleanup()` before `_stage.reset()`.
- Toggled at runtime by the settings window (step 05) using the same
  `start()/stop()` pair.

## Verification

Build, then (server enabled):

```sh
curl -s http://127.0.0.1:8643/status
curl -s -X POST http://127.0.0.1:8643/import -H 'Content-Type: application/json' \
     -d '{"fileName":"x.glb","filePath":"/nope.glb","units":"m","coordinateSystem":"y_up"}'
# -> 404 {"status":"error","message":"File not found: /nope.glb"}
```

(Real import results are verified in step 07.)

## Notes / risks

- `httplib.h` pulls in sockets + `<thread>` machinery; keeping it in one TU
  contains compile time and prevents macro leakage.
- The 60 s timeout bounds worst-case latency for huge glTF files; the scene is
  loaded synchronously on the main thread (same as the existing menu import).
- A second `start()` after `stop()` uses a fresh `httplib::Server` (routes are
  re-registered) — avoids reusing a decommissioned instance.
