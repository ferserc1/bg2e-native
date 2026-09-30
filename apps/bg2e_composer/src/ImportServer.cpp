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
#include "ImportServer.hpp"

#include <httplib.h>                      // sole includer of httplib.h
#include <bg2e/json/JsonParser.hpp>       // engine JSON parser
#include <bg2e/json/JsonNode.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <utility>

struct ImportServer::Impl {
    // Recreated on every start(): httplib::Server owns atomics/threads and
    // cannot be re-assigned once decommissioned by stop().
    std::unique_ptr<httplib::Server> server;
    std::thread listenThread;
    uint32_t boundPort = 0;
};

namespace {

std::string lowerCase(const std::string& text)
{
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

// Minimal JSON string escaping for the message field of responses.
std::string jsonEscape(const std::string& text)
{
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                }
                else
                {
                    out += c;
                }
        }
    }
    return out;
}

void fail(httplib::Response& res, int status, const std::string& message)
{
    res.status = status;
    res.set_content("{\"status\":\"error\",\"message\":\"" + jsonEscape(message) + "\"}",
                    "application/json");
}

bool unitsToScale(const std::string& units, float& scale)
{
    if (units == "m")  { scale = 1.0f;    return true; }
    if (units == "cm") { scale = 0.01f;   return true; }
    if (units == "mm") { scale = 0.001f;  return true; }
    if (units == "in") { scale = 0.0254f; return true; }
    if (units == "ft") { scale = 0.3048f; return true; }
    return false;
}

}

ImportServer::ImportServer() :
    _impl(std::make_unique<Impl>())
{
}

ImportServer::~ImportServer()
{
    stop();
}

std::string ImportServer::start(uint32_t port)
{
    if (_impl->listenThread.joinable()) return "Service already running";
    if (port < 1024 || port > 49151) return "Port out of valid range (1024-49151)";

    _impl->server = std::make_unique<httplib::Server>();
    auto& svr = *_impl->server;

    // Make an occupied port fail the bind (default SO_REUSEPORT would share it).
    // socket_t is a global typedef in this httplib version (defined before
    // namespace httplib), so the parameter type must stay unqualified.
    svr.set_socket_options([](socket_t sock) {
    #ifdef _WIN32
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
    #else
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_REUSEADDR, 1);
    #endif
    });

    // One handler may wait for import completion; others answer busy/status.
    svr.new_task_queue = [] { return new httplib::ThreadPool(4, 32); };

    svr.Get("/status", [this](const httplib::Request&, httplib::Response& res) {
        std::string body = std::string("{\"running\":true,\"port\":") +
            std::to_string(_impl->boundPort) +
            ",\"busy\":" + (busy() ? "true" : "false") + "}";
        res.set_content(body, "application/json");
    });

    svr.Post("/import", [this](const httplib::Request& req, httplib::Response& res) {
        handleImport(req, res);
    });

    if (!svr.bind_to_port("127.0.0.1", static_cast<int>(port)))
    {
        _impl->server.reset();
        return "Port " + std::to_string(port) + " is already in use";
    }
    _impl->boundPort = port;
    {
        std::lock_guard lock(_mutex);
        _accepting = true;
    }

    _impl->listenThread = std::thread([&svr]() {
        svr.listen_after_bind();           // blocks until stop()
    });
    svr.wait_until_ready();
    return "";
}

void ImportServer::stop()
{
    {
        std::lock_guard lock(_mutex);
        _accepting = false;
        if (_slot)
        {
            _slot->done = true;
            _slot->ok = false;
            _slot->message = "Service stopped";
            _slot.reset();
        }
    }
    _cv.notify_all();
    if (_impl->listenThread.joinable())
    {
        _impl->server->stop();             // noexcept, wakes the accept loop
        _impl->listenThread.join();
    }
    _impl->server.reset();                 // fresh instance for the next start()
    _impl->boundPort = 0;

}

bool ImportServer::isRunning() const
{
    return _impl->listenThread.joinable();
}

uint32_t ImportServer::port() const
{
    return _impl->boundPort;
}

bool ImportServer::takePending(ImportRequest& request)
{
    std::lock_guard lock(_mutex);
    if (!_slot || _slot->taken || _slot->done) return false;
    _slot->taken = true;
    request = _slot->request;
    return true;
}

void ImportServer::fulfil(uint64_t id, bool ok, const std::string& message)
{
    {
        std::lock_guard lock(_mutex);
        if (!_slot || _slot->request.id != id || _slot->done) return;
        _slot->done = true;
        _slot->ok = ok;
        _slot->message = message;
        _slot.reset();
    }
    _cv.notify_all();
}

bool ImportServer::busy() const
{
    std::lock_guard lock(_mutex);
    return static_cast<bool>(_slot);
}

void ImportServer::handleImport(const httplib::Request& req, httplib::Response& res)
{
    if (busy())
    {
        fail(res, 503, "Import service busy");
        return;
    }
    if (lowerCase(req.get_header_value("Content-Type")).find("application/json") ==
        std::string::npos)
    {
        fail(res, 415, "Content-Type must be application/json");
        return;
    }

    std::shared_ptr<bg2e::json::JsonNode> root;
    try
    {
        root = bg2e::json::JsonParser(req.body).parse();
    }
    catch (...)
    {
        root = nullptr;
    }
    if (!root || !root->isObject())
    {
        fail(res, 400, "Malformed JSON body");
        return;
    }

    ImportRequest r;

    r.filePath = root->objectValue("filePath").stringValue("");
    if (r.filePath.empty())
    {
        fail(res, 400, "Missing required field: filePath");
        return;
    }

    r.fileName = root->objectValue("fileName").stringValue("");
    if (r.fileName.empty())
    {
        r.fileName = r.filePath.filename().string();
    }

    auto& unitsNode = root->objectValue("units");
    const std::string units = unitsNode.isNull() ? "m" : lowerCase(unitsNode.stringValue(""));
    if (!unitsToScale(units, r.unitsScale))
    {
        fail(res, 400, "Unknown units: " + units);
        return;
    }

    auto& coordNode = root->objectValue("coordinateSystem");
    const std::string coordSystem = coordNode.isNull() ? "y_up" : lowerCase(coordNode.stringValue(""));
    if (coordSystem == "y_up")
    {
        r.coordinateSystem = ImportCoordinateSystem::YUp;
    }
    else if (coordSystem == "z_up")
    {
        r.coordinateSystem = ImportCoordinateSystem::ZUp;
    }
    else
    {
        fail(res, 400, "Unknown coordinateSystem: " + coordSystem);
        return;
    }

    if (!std::filesystem::exists(r.filePath))
    {
        fail(res, 404, "File not found: " + r.filePath.string());
        return;
    }
    auto ext = lowerCase(r.filePath.extension().string());
    if (ext != ".gltf" && ext != ".glb")
    {
        fail(res, 400, "Only .gltf/.glb files are supported");
        return;
    }

    std::shared_ptr<Slot> slot;
    {
        std::lock_guard lock(_mutex);
        if (!_accepting || _slot)
        {
            fail(res, 503, "Import service busy");
            return;
        }
        slot = std::make_shared<Slot>();
        r.id = _nextId++;
        slot->request = std::move(r);
        _slot = slot;
    }

    bool ok = false;
    std::string message;
    {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [&slot] { return slot->done; });
        ok = slot->ok;
        message = slot->message;
    }

    if (!ok)
    {
        fail(res, 500, message);
        return;
    }

    res.status = 200;
    res.set_content("{\"status\":\"ok\",\"message\":\"" + jsonEscape(message) + "\"}",
                    "application/json");
}
