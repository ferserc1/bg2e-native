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

#include <bg2e/plugin/PluginManager.hpp>
#include <bg2e/plugin/ComponentRegistrar.hpp>
#include <bg2e/plugin/PluginComponentsVisitor.hpp>
#include <bg2e/base/DynamicLibrary.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/scene/Node.hpp>

#include "ModuleState.hpp"

#include <algorithm>
#include <cstring>
#include <cctype>
#include <exception>
#include <new>
#include <stdexcept>
#include <thread>
#include <utility>

namespace bg2e::plugin {

namespace {

bool candidate(const std::filesystem::path& path)
{
    const auto extension = path.extension().string();
#ifdef _WIN32
    auto lowered = extension;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character)
    {
        return char(std::tolower(character));
    });
    return lowered == ".dll";
#elif defined(__APPLE__)
    return extension == ".dylib" || extension == ".so" || extension == ".bundle";
#else
    return extension == ".so";
#endif
}

void* symbol(const base::DynamicLibrary& library, const char* name)
{
    try
    {
        return library.getSymbol(name);
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (...)
    {
        return nullptr;
    }
}

void compare(const PluginApi& plugin, LoadReport& report)
{
    const auto& host = engineSdk();
    if (std::strcmp(host.architecture, "unsupported") == 0 ||
        std::strcmp(host.compiler, "unsupported") == 0 ||
        std::strcmp(host.standardLibrary, "unsupported") == 0)
    {
        report.errors.emplace_back("This engine toolchain/architecture has no supported plugin ABI fingerprint");
    }
    const auto text = [&report](const char* label, const char* expected, const char* actual)
    {
        if (!actual || !expected || std::strcmp(expected, actual) != 0)
        {
            report.errors.push_back(std::string(label) + ": host=" + (expected ? expected : "<null>")
                + ", plugin=" + (actual ? actual : "<null>"));
        }
    };
    text("SDK version", host.sdkVersion, plugin.sdkVersion);
    text("SDK ABI", host.sdkAbi, plugin.sdkAbi);
    text("Architecture", host.architecture, plugin.architecture);
    text("Compiler", host.compiler, plugin.compiler);
    text("Standard library ABI", host.standardLibrary, plugin.standardLibrary);
    text("Runtime", host.runtime, plugin.runtime);
    text("Build ABI options", host.buildOptions, plugin.buildOptions);
    const auto number = [&report](const char* label, auto expected, auto actual)
    {
        if (expected != actual)
        {
            report.errors.push_back(std::string(label) + ": host=" + std::to_string(expected)
                + ", plugin=" + std::to_string(actual));
        }
    };
    number("Pointer size", host.pointerSize, plugin.pointerSize);
    number("std::string size", host.stringSize, plugin.stringSize);
    number("std::shared_ptr size", host.sharedPointerSize, plugin.sharedPointerSize);
    number("Compiler ABI version", host.compilerAbi, plugin.compilerAbi);
    number("Standard library hardening mode", host.hardeningMode, plugin.hardeningMode);
    number("Packing", host.packing, plugin.packing);
    number("RTTI/exceptions/stdlib flags", host.flags, plugin.flags);
    number("Iterator debug level", host.iteratorDebugLevel, plugin.iteratorDebugLevel);
    number("C++ language version", host.cppVersion, plugin.cppVersion);
    if (plugin.sharedEngine != host.sharedEngine)
    {
        report.errors.emplace_back("The plugin does not resolve the application's shared engine");
    }
    if (!plugin.destroy)
    {
        report.errors.emplace_back("The plugin destruction entry point is null");
    }
}

void warn(const LoadReport& report) noexcept
{
    try
    {
        for (const auto& type : report.duplicates)
        {
            bg2e_log_warning << "Plugin duplicate omitted: " << type << bg2e_log_end;
        }
    }
    catch (...)
    {
    }
}

struct BusyScope {
    bool& busy;
    explicit BusyScope(bool& value) : busy(value)
    {
        busy = true;
    }

    ~BusyScope()
    {
        busy = false;
    }
};

}

struct PluginManager::Impl {
    struct Record {
        base::DynamicLibrary library;
        std::filesystem::path path;
        std::shared_ptr<detail::ModuleState> state;
        Plugin* instance = nullptr;
        void (BG2E_PLUGIN_CALL *destroy)(void*) = nullptr;
        PluginInfo info;
        SessionId session = 0;
        bool stopped = false;
        bool prepared = false;
        bool retired = false;
        bool finished = false;
        bool failed = false;
        bool callbackFailed = false;
        bool released = false;
        std::vector<std::function<void()>> stopCallbacks;
    };

    struct RegistryReader {
        std::atomic<uint32_t>& readers;
        explicit RegistryReader(std::atomic<uint32_t>& value) : readers(value)
        {
            readers.fetch_add(1);
        }

        ~RegistryReader()
        {
            readers.fetch_sub(1);
        }
    };

    std::thread::id thread = std::this_thread::get_id();
    std::atomic<uint32_t> readers{0};
    bool busy = false;
    SessionId nextSession = 1;
    std::vector<std::unique_ptr<Record>> records;

    bool onThread() const
    {
        return thread == std::this_thread::get_id();
    }

    Record* find(PluginId id) const
    {
        for (const auto& record : records)
        {
            if (record->state->id == id)
            {
                return record.get();
            }
        }
        return nullptr;
    }

    PluginUnloadContext context(const Record& record, PluginServices services, UnloadReason reason) const
    {
        PluginUnloadContext result;
        result.handle = record.state->id;
        result.pluginId = record.info.id;
        result.path = record.path;
        result.services = services;
        result.reason = reason;
        return result;
    }

    void stop(Record& record) noexcept
    {
        if (record.stopped)
        {
            return;
        }
        record.stopped = true;
        record.state->accepting.store(false);
        for (auto& stop : record.stopCallbacks)
        {
            try
            {
                stop();
            }
            catch (...)
            {
                record.callbackFailed = true;
            }
        }
        record.stopCallbacks.clear();
    }

    void prepare(Record& record, PluginServices services, UnloadReason reason) noexcept
    {
        stop(record);
        if (readers.load() != 0)
        {
            record.callbackFailed = true;
            return;
        }
        if (record.prepared)
        {
            return;
        }
        record.prepared = true;
        try
        {
            if (record.instance)
            {
                record.instance->onPrepareUnload(context(record, services, reason));
            }
        }
        catch (...)
        {
            record.callbackFailed = true;
        }
    }

    void retire(Record& record) noexcept
    {
        if (record.retired)
        {
            return;
        }
        if (readers.load() != 0)
        {
            record.callbackFailed = true;
            return;
        }
        // Removal is intentionally allocation-free so initialization rollback
        // remains possible when building an error report runs out of memory.
        scene::ComponentFactoryRegistry::get().removeOwner(record.state->id);
        reflection::TypeRegistry::get().removeOwner(record.state->id);
        record.retired = true;
    }

    void finish(Record& record, PluginServices services, UnloadReason reason) noexcept
    {
        if (record.finished || !record.retired)
        {
            return;
        }
        record.finished = true;
        try
        {
            if (record.instance)
            {
                record.instance->onUnload(context(record, services, reason));
            }
        }
        catch (...)
        {
            record.callbackFailed = true;
        }
        if (record.instance)
        {
            auto instance = std::exchange(record.instance, nullptr);
            try
            {
                record.destroy(instance);
            }
            catch (...)
            {
                record.callbackFailed = true;
            }
        }
        // Native release is a separate host operation after all callbacks and
        // destructors return. Unknown external uses are the caller's responsibility.
    }

    bool release(Record& record) noexcept
    {
        if (record.released)
        {
            return true;
        }
        if (!record.finished || !record.retired || record.instance ||
            record.callbackFailed || record.state.use_count() != 1)
        {
            return false;
        }
        // The caller promises no live external objects, weak control blocks,
        // callbacks or plugin stacks. Registry contributions and the instance
        // have already been destroyed while the module was still mapped.
        record.library.unload();
        record.released = true;
        return true;
    }

    void rollback(Record& record) noexcept
    {
        record.failed = true;
        prepare(record, {nullptr, true}, UnloadReason::InitializationFailure);
        retire(record);
        finish(record, {nullptr, true}, UnloadReason::InitializationFailure);
    }

    LoadReport load(const std::filesystem::path& path, SessionId session)
    {
        LoadReport report;
        report.path = path;
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error))
        {
            report.status = LoadStatus::FilesystemFailure;
            report.message = error ? error.message() : "Plugin path is not a regular file";
            return report;
        }
        if (!candidate(path))
        {
            report.status = LoadStatus::UnsupportedFile;
            return report;
        }
        auto canonical = std::filesystem::canonical(path, error);
        if (error)
        {
            report.status = LoadStatus::FilesystemFailure;
            report.message = error.message();
            return report;
        }
        report.path = canonical;
        for (const auto& record : records)
        {
            if (record->path == canonical && !record->retired)
            {
                report.handle = record->state->id;
                report.pluginId = record->info.id;
                report.status = LoadStatus::AlreadyLoaded;
                return report;
            }
        }
        auto record = std::make_unique<Record>();
        try
        {
            record->library = base::DynamicLibrary::load(canonical);
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const std::exception& exception)
        {
            report.status = LoadStatus::LibraryLoadFailure;
            report.message = exception.what();
            return report;
        }
        auto create = reinterpret_cast<CreatePlugin>(symbol(record->library, "getPluginInstance"));
        if (!create)
        {
            report.status = LoadStatus::NotAPlugin;
            return report;
        }
        auto query = reinterpret_cast<QueryPluginApi>(symbol(record->library, "getPluginApi"));
        if (!query)
        {
            report.status = LoadStatus::Incompatible;
            report.message = "Missing getPluginApi descriptor query";
            return report;
        }
        // Query exceptions are destroyed before the local library owner leaves
        // this scope. The same rule applies to every plugin callback below.
        const PluginApi* api = nullptr;
        bool queryFailed = false;
        try
        {
            api = query();
        }
        catch (...)
        {
            queryFailed = true;
        }
        if (queryFailed)
        {
            report.status = LoadStatus::InitializationFailure;
            report.message = "Plugin descriptor query threw an exception";
            return report;
        }
        if (!api || api->structureSize != sizeof(PluginApi) ||
            api->contractVersion != BG2E_PLUGIN_CONTRACT_VERSION)
        {
            report.status = LoadStatus::Incompatible;
            report.message = "Null descriptor or unsupported descriptor size/contract version";
            return report;
        }
        compare(*api, report);
        if (!report.errors.empty())
        {
            report.status = LoadStatus::Incompatible;
            return report;
        }
        record->destroy = api->destroy;
        record->path = canonical;
        record->session = session;
        record->state = std::make_shared<detail::ModuleState>();
        record->state->id = base::allocateRegistrationOwner();
        auto* active = record.get();
        // Establish host ownership before the first C++ plugin call. Rollback
        // destroys contributions/instances before later safe-point collection;
        // plugin constructors and callbacks must not leak external native uses.
        records.push_back(std::move(record));
        report.handle = active->state->id;
        try
        {
            active->instance = static_cast<Plugin*>(create());
            if (!active->instance)
            {
                throw std::runtime_error("Plugin factory returned null");
            }
            active->info = active->instance->info();
            report.pluginId = active->info.id;
            if (active->info.id.empty() || active->info.name.empty())
            {
                throw std::runtime_error("Plugin identity and display name must be nonempty");
            }
            if (!active->info.requiredSdkAbi.empty() && active->info.requiredSdkAbi != engineSdk().sdkAbi)
            {
                report.status = LoadStatus::Incompatible;
                report.message = "Plugin metadata requires another SDK ABI";
                rollback(*active);
                return report;
            }
            for (const auto& other : records)
            {
                if (other.get() != active && !other->retired && !other->failed && other->info.id == active->info.id)
                {
                    report.status = LoadStatus::AlreadyLoaded;
                    report.handle = other->state->id;
                    rollback(*active);
                    return report;
                }
            }
            ComponentRegistrar registrar(ModuleHandle(active->state), report, active->stopCallbacks,
                active->instance->_componentTypes);
            PluginLoadContext context;
            context.handle = active->state->id;
            context.pluginId = active->info.id;
            context.path = active->path;
            active->instance->registerComponents(registrar, context);
            report.status = report.duplicates.empty() && report.errors.empty()
                ? LoadStatus::Loaded : LoadStatus::LoadedWithWarnings;
        }
        catch (const std::bad_alloc&)
        {
            // The exception object still exists here: native release is deferred.
            rollback(*active);
            throw;
        }
        catch (const std::exception& exception)
        {
            report.status = LoadStatus::InitializationFailure;
            try
            {
                report.message = exception.what();
            }
            catch (...)
            {
                rollback(*active);
                throw;
            }
            rollback(*active);
        }
        catch (...)
        {
            rollback(*active);
            report.status = LoadStatus::InitializationFailure;
            report.message = "Plugin factory/metadata/registration callback failed";
        }
        return report;
    }

    ScanReport scan(const std::filesystem::path& directory, bool missingIsEmpty, SessionId session)
    {
        ScanReport report;
        std::error_code error;
        const auto status = std::filesystem::status(directory, error);
        if (missingIsEmpty && (error == std::errc::no_such_file_or_directory ||
            (!error && !std::filesystem::exists(status))))
        {
            return report;
        }
        if (error || !std::filesystem::is_directory(status))
        {
            report.status = LoadStatus::FilesystemFailure;
            report.message = error ? error.message() : "Plugin scan path is not a directory";
            return report;
        }
        std::vector<std::filesystem::path> paths;
        std::filesystem::directory_iterator iterator(directory, error), end;
        while (!error && iterator != end)
        {
            const auto path = iterator->path();
            if (candidate(path))
            {
                paths.push_back(path);
            }
            iterator.increment(error);
        }
        if (error)
        {
            report.status = LoadStatus::FilesystemFailure;
            report.message = error.message();
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& path : paths)
        {
            try
            {
                report.files.push_back(load(path, session));
            }
            catch (const std::bad_alloc&)
            {
                throw;
            }
            catch (const std::exception& exception)
            {
                LoadReport failure;
                failure.path = path;
                failure.status = LoadStatus::InitializationFailure;
                failure.message = exception.what();
                report.files.push_back(std::move(failure));
            }
            catch (...)
            {
                LoadReport failure;
                failure.path = path;
                failure.status = LoadStatus::InitializationFailure;
                report.files.push_back(std::move(failure));
            }
        }
        return report;
    }
};

PluginManager::PluginManager() : _impl(std::make_unique<Impl>())
{
    // Initialize rollback services before any user code or module can run.
    scene::ComponentFactoryRegistry::get();
    reflection::TypeRegistry::get();
}

PluginManager::~PluginManager() = default;

PluginManager& PluginManager::get()
{
    // Host records/identifiers persist for the process. Native mappings are
    // explicitly released after the programmer-owned cleanup contract is met.
    static auto* manager = new PluginManager();
    return *manager;
}

LoadReport PluginManager::loadPlugin(const std::filesystem::path& path) noexcept
{
    std::vector<std::string> duplicates;
    auto report = loadPlugin(path, duplicates);
    warn(report);
    return report;
}

LoadReport PluginManager::loadPlugin(const std::filesystem::path& path,
    std::vector<std::string>& duplicates) noexcept
{
    LoadReport report;
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        report.status = !_impl->onThread() ? LoadStatus::WrongThread : LoadStatus::Busy;
        return report;
    }
    BusyScope scope(_impl->busy);
    try
    {
        report = _impl->load(path, 0);
        duplicates.insert(duplicates.end(), report.duplicates.begin(), report.duplicates.end());
    }
    catch (const std::bad_alloc&)
    {
        report.status = LoadStatus::OutOfMemory;
    }
    catch (const std::exception& exception)
    {
        report.status = LoadStatus::InitializationFailure;
        try
        {
            report.message = exception.what();
        }
        catch (...)
        {
        }
    }
    catch (...)
    {
        report.status = LoadStatus::InitializationFailure;
    }
    return report;
}

ScanReport PluginManager::loadPlugins(const std::filesystem::path& directory, bool missingIsEmpty) noexcept
{
    std::vector<std::string> duplicates;
    auto report = loadPlugins(directory, duplicates, missingIsEmpty);
    for (const auto& file : report.files)
    {
        warn(file);
    }
    return report;
}

ScanReport PluginManager::loadPlugins(const std::filesystem::path& directory,
    std::vector<std::string>& duplicates, bool missingIsEmpty) noexcept
{
    ScanReport report;
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        report.status = !_impl->onThread() ? LoadStatus::WrongThread : LoadStatus::Busy;
        return report;
    }
    BusyScope scope(_impl->busy);
    try
    {
        report = _impl->scan(directory, missingIsEmpty, 0);
        for (const auto& file : report.files)
        {
            duplicates.insert(duplicates.end(), file.duplicates.begin(), file.duplicates.end());
        }
    }
    catch (const std::bad_alloc&)
    {
        report.status = LoadStatus::OutOfMemory;
    }
    catch (...)
    {
        report.status = LoadStatus::InitializationFailure;
    }
    return report;
}

const PluginInfo* PluginManager::info(PluginId id) const
{
    if (!_impl->onThread())
    {
        return nullptr;
    }
    const auto* record = _impl->find(id);
    return record && !record->failed ? &record->info : nullptr;
}

ModuleHandle PluginManager::module(PluginId id) const
{
    if (!_impl->onThread())
    {
        return {};
    }
    const auto* record = _impl->find(id);
    return record && !record->retired ? ModuleHandle(record->state) : ModuleHandle{};
}

std::vector<PluginId> PluginManager::plugins() const
{
    std::vector<PluginId> result;
    if (_impl->onThread())
    {
        for (const auto& record : _impl->records)
        {
            if (!record->retired)
            {
                result.push_back(record->state->id);
            }
        }
    }
    return result;
}

size_t PluginManager::removeComponents(PluginId id, scene::Node* root)
{
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        throw std::logic_error("Plugin scene cleanup requires the manager thread outside callbacks and background reads");
    }
    auto* record = _impl->find(id);
    if (!record || record->retired || !record->instance)
    {
        throw std::invalid_argument("Plugin scene cleanup requires a loaded plugin");
    }
    BusyScope scope(_impl->busy);
    return record->instance->removeComponents(root);
}

std::vector<std::string> PluginManager::componentTypes(PluginId id) const
{
    if (!_impl->onThread())
    {
        throw std::logic_error("Plugin component enumeration requires the manager thread");
    }
    const auto* record = _impl->find(id);
    return record && record->instance ? record->instance->componentTypes() : std::vector<std::string>{};
}

std::shared_ptr<void> PluginManager::registryReadLease()
{
    if (!_impl->onThread() || _impl->busy)
    {
        throw std::logic_error("Registry read leases must be acquired on the manager thread outside callbacks");
    }
    return std::make_shared<Impl::RegistryReader>(_impl->readers);
}

UnloadReport PluginManager::unload(PluginId id, const UnloadOptions& options) noexcept
{
    UnloadReport report;
    report.handle = id;
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        report.status = !_impl->onThread() ? UnloadStatus::WrongThread : UnloadStatus::Busy;
        return report;
    }
    BusyScope scope(_impl->busy);
    auto* record = _impl->find(id);
    if (!record)
    {
        report.status = UnloadStatus::NotFound;
        return report;
    }
    try
    {
        if (!options.services.stopped)
        {
            report.status = UnloadStatus::Busy;
            report.message = "Unload requires stopped frame/background producers and drained GPU work";
            return report;
        }
        if (record->retired)
        {
            report.status = _impl->release(*record) ? UnloadStatus::Unloaded
                : (record->callbackFailed ? UnloadStatus::Failed : UnloadStatus::Pending);
            return report;
        }
        PluginComponentsVisitor visitor(record->instance->componentTypes());
        for (const auto& root : options.roots)
        {
            if (root)
            {
                root->accept(&visitor);
            }
        }
        if (options.policy == UnloadPolicy::RefuseIfInUse &&
            !visitor.entries().empty())
        {
            report.status = UnloadStatus::InUse;
            report.message = "The supplied roots still contain plugin components; clean every active scene first";
            return report;
        }
        _impl->prepare(*record, options.services, UnloadReason::Runtime);
        if (options.policy == UnloadPolicy::RemoveComponents)
        {
            visitor.removeComponents();
        }
        if (options.releaseReferences)
        {
            options.releaseReferences();
        }
        _impl->retire(*record);
        _impl->finish(*record, options.services, UnloadReason::Runtime);
        report.status = record->callbackFailed ? UnloadStatus::Failed
            : (_impl->release(*record) ? UnloadStatus::Unloaded : UnloadStatus::Pending);
        if (report.status == UnloadStatus::Pending)
        {
            report.message = "Explicit module leases remain; release them and call collect at a host safe point";
        }
    }
    catch (const std::bad_alloc&)
    {
        report.status = UnloadStatus::OutOfMemory;
    }
    catch (...)
    {
        report.status = UnloadStatus::Failed;
    }
    return report;
}

SessionId PluginManager::beginSession()
{
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        throw std::logic_error("Plugin sessions must begin on the manager thread outside callbacks");
    }
    return _impl->nextSession++;
}

ScanReport PluginManager::discover(SessionId session) noexcept
{
    ScanReport report;
    if (!_impl->onThread() || _impl->busy || !session || _impl->readers.load() != 0)
    {
        report.status = !_impl->onThread() ? LoadStatus::WrongThread : LoadStatus::Busy;
        return report;
    }
    BusyScope scope(_impl->busy);
    try
    {
        report = _impl->scan(base::PlatformTools::pluginPath(), true, session);
        for (const auto& file : report.files)
        {
            warn(file);
            if (!file.loaded() && file.status != LoadStatus::NotAPlugin)
            {
                bg2e_log_warning << "Plugin loading failed: " << file.path.string()
                    << " " << file.message << bg2e_log_end;
            }
            for (const auto& detail : file.errors)
            {
                bg2e_log_warning << "Plugin " << file.path.string() << ": " << detail << bg2e_log_end;
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        report.status = LoadStatus::OutOfMemory;
    }
    catch (...)
    {
        report.status = LoadStatus::FilesystemFailure;
    }
    return report;
}

void PluginManager::stopSession(SessionId session) noexcept
{
    if (!_impl->onThread() || _impl->busy || !session)
    {
        return;
    }
    BusyScope scope(_impl->busy);
    for (auto it = _impl->records.rbegin(); it != _impl->records.rend(); ++it)
    {
        if ((*it)->session == session)
        {
            _impl->stop(**it);
        }
    }
}

void PluginManager::prepareSession(SessionId session, PluginServices services, UnloadReason reason) noexcept
{
    if (!_impl->onThread() || _impl->busy || !session)
    {
        return;
    }
    BusyScope scope(_impl->busy);
    for (auto it = _impl->records.rbegin(); it != _impl->records.rend(); ++it)
    {
        if ((*it)->session == session)
        {
            _impl->prepare(**it, services, reason);
        }
    }
}

void PluginManager::retireSession(SessionId session) noexcept
{
    if (!_impl->onThread() || _impl->busy || !session)
    {
        return;
    }
    BusyScope scope(_impl->busy);
    for (auto it = _impl->records.rbegin(); it != _impl->records.rend(); ++it)
    {
        if ((*it)->session == session)
        {
            _impl->retire(**it);
        }
    }
}

void PluginManager::finishSession(SessionId session, UnloadReason reason) noexcept
{
    if (!_impl->onThread() || _impl->busy || !session)
    {
        return;
    }
    BusyScope scope(_impl->busy);
    for (auto it = _impl->records.rbegin(); it != _impl->records.rend(); ++it)
    {
        if ((*it)->session == session)
        {
            auto& record = **it;
            _impl->prepare(record, {nullptr, true}, reason);
            _impl->retire(record);
            _impl->finish(record, {nullptr, true}, reason);
            if (record.state.use_count() > 1 || record.callbackFailed)
            {
                try
                {
                    bg2e_log_warning << "Plugin module pending explicit leases or failed cleanup: "
                        << record.path.string() << bg2e_log_end;
                }
                catch (...)
                {
                }
            }
        }
    }
}

void PluginManager::collect() noexcept
{
    if (!_impl->onThread() || _impl->busy || _impl->readers.load() != 0)
    {
        return;
    }
    BusyScope scope(_impl->busy);
    for (auto it = _impl->records.rbegin(); it != _impl->records.rend(); ++it)
    {
        auto& record = **it;
        _impl->release(record);
    }
}

Plugin::~Plugin() = default;

const std::vector<std::string>& Plugin::componentTypes() const noexcept
{
    return _componentTypes;
}

size_t Plugin::removeComponents(scene::Node* root) const
{
    if (!root)
    {
        return 0;
    }
    PluginComponentsVisitor visitor(_componentTypes);
    root->accept(&visitor);
    return visitor.removeComponents();
}

size_t Plugin::removeComponents(PluginId id, scene::Node* root)
{
    return PluginManager::get().removeComponents(id, root);
}

LoadReport Plugin::loadPlugin(const std::filesystem::path& path) noexcept
{
    try
    {
        return PluginManager::get().loadPlugin(path);
    }
    catch (...)
    {
        LoadReport report;
        report.status = LoadStatus::OutOfMemory;
        return report;
    }
}

LoadReport Plugin::loadPlugin(const std::filesystem::path& path, std::vector<std::string>& duplicates) noexcept
{
    try
    {
        return PluginManager::get().loadPlugin(path, duplicates);
    }
    catch (...)
    {
        LoadReport report;
        report.status = LoadStatus::OutOfMemory;
        return report;
    }
}

ScanReport Plugin::loadPlugins(const std::filesystem::path& directory) noexcept
{
    try
    {
        return PluginManager::get().loadPlugins(directory);
    }
    catch (...)
    {
        ScanReport report;
        report.status = LoadStatus::OutOfMemory;
        return report;
    }
}

ScanReport Plugin::loadPlugins(const std::filesystem::path& directory, std::vector<std::string>& duplicates) noexcept
{
    try
    {
        return PluginManager::get().loadPlugins(directory, duplicates);
    }
    catch (...)
    {
        ScanReport report;
        report.status = LoadStatus::OutOfMemory;
        return report;
    }
}

}
