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

#include <bg2e/common.hpp>
#include <bg2e/plugin/BuildConfig.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

#define BG2E_PLUGIN_CONTRACT_VERSION 1u
#define BG2E_SDK_VERSION "0.1.0"
#define BG2E_SDK_ABI_ID "bg2e-plugin-sdk-2"
// Increment SDK_ABI_ID whenever a public C++ layout/contract changes. The CMake
// helper supplies additional ABI-sensitive options identically for both targets.
#ifndef BG2E_SDK_BUILD_ABI
#define BG2E_SDK_BUILD_ABI "default"
#endif

#define BG2E_PLUGIN_STRINGIFY_IMPL(value) #value
#define BG2E_PLUGIN_STRINGIFY(value) BG2E_PLUGIN_STRINGIFY_IMPL(value)

#ifdef _WIN32
#define BG2E_PLUGIN_CALL __cdecl
#define BG2E_PLUGIN_EXPORT __declspec(dllexport)
#else
#define BG2E_PLUGIN_CALL
#define BG2E_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace bg2e::plugin {

// Bootstrap data contains no C++ objects. Exact size and contract version are
// required in v1. Strings are immutable, null-terminated module-owned literals.
#pragma pack(push, 8)
struct PluginApi {
    uint32_t structureSize;
    uint32_t contractVersion;
    const char* sdkVersion;
    const char* sdkAbi;
    const char* architecture;
    const char* compiler;
    const char* standardLibrary;
    const char* runtime;
    const char* buildOptions;
    uint32_t pointerSize;
    uint32_t packing;
    uint32_t flags;
    uint32_t iteratorDebugLevel;
    uint64_t cppVersion;
    uint32_t stringSize;
    uint32_t sharedPointerSize;
    uint32_t compilerAbi;
    uint32_t hardeningMode;
    const void* sharedEngine;
    void (BG2E_PLUGIN_CALL *destroy)(void*);
};
#pragma pack(pop)

using QueryPluginApi = const PluginApi* (BG2E_PLUGIN_CALL *)();
using CreatePlugin = void* (BG2E_PLUGIN_CALL *)();

BG2E_API const PluginApi& engineSdk();
BG2E_API const void* sharedEngineIdentity() noexcept;

namespace detail {

struct PackingProbe {
    char first;
    uint64_t second;
};

// Internal linkage is essential: do not let linker symbol interposition replace
// the module's fingerprint with the engine's inline definition.
static inline PluginApi localSdk()
{
    PluginApi api{};
    api.structureSize = sizeof(PluginApi);
    api.contractVersion = BG2E_PLUGIN_CONTRACT_VERSION;
    api.sdkVersion = BG2E_SDK_VERSION;
    api.sdkAbi = BG2E_SDK_ABI_ID;
#if defined(__aarch64__) || defined(_M_ARM64)
    api.architecture = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    api.architecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    api.architecture = "x86";
#else
    api.architecture = "unsupported";
#endif
#if defined(__clang__)
    api.compiler = "clang-" __clang_version__;
#elif defined(_MSC_VER)
    api.compiler = "msvc-" BG2E_PLUGIN_STRINGIFY(_MSC_FULL_VER);
#elif defined(__GNUC__)
    api.compiler = "gcc-" __VERSION__;
#else
    api.compiler = "unsupported";
#endif
#if defined(_LIBCPP_VERSION)
    api.standardLibrary = "libc++-" BG2E_PLUGIN_STRINGIFY(_LIBCPP_VERSION)
        "-abi-" BG2E_PLUGIN_STRINGIFY(_LIBCPP_ABI_VERSION);
#elif defined(__GLIBCXX__)
    api.standardLibrary = "libstdc++-" BG2E_PLUGIN_STRINGIFY(__GLIBCXX__)
        "-cxx11-" BG2E_PLUGIN_STRINGIFY(_GLIBCXX_USE_CXX11_ABI);
#elif defined(_MSVC_STL_VERSION)
    api.standardLibrary = "msvc-stl-" BG2E_PLUGIN_STRINGIFY(_MSVC_STL_VERSION)
        "-" BG2E_PLUGIN_STRINGIFY(_MSVC_STL_UPDATE);
#else
    api.standardLibrary = "unsupported";
#endif
#if defined(_WIN32) && defined(_DLL)
#ifdef _DEBUG
    api.runtime = "windows-dynamic-debug-crt";
#else
    api.runtime = "windows-dynamic-crt";
#endif
#elif defined(_WIN32)
    api.runtime = "windows-static-crt";
#elif defined(__APPLE__)
    api.runtime = "darwin";
#else
    api.runtime = "linux";
#endif
    api.buildOptions = BG2E_SDK_BUILD_ABI;
    api.stringSize = sizeof(std::string);
    api.sharedPointerSize = sizeof(std::shared_ptr<void>);
#ifdef __GXX_ABI_VERSION
    api.compilerAbi = __GXX_ABI_VERSION;
#endif
#ifdef _LIBCPP_HARDENING_MODE
    api.hardeningMode = _LIBCPP_HARDENING_MODE;
#endif
    api.pointerSize = sizeof(void*);
    api.packing = offsetof(PackingProbe, second);
#if defined(__GXX_RTTI) || defined(_CPPRTTI)
    api.flags |= 1u;
#endif
#if defined(__EXCEPTIONS) || defined(_CPPUNWIND)
    api.flags |= 2u;
#endif
#ifdef _GLIBCXX_DEBUG
    api.flags |= 4u;
#endif
#ifdef _GLIBCXX_ASSERTIONS
    api.flags |= 16u;
#endif
#ifdef _LIBCPP_ABI_UNSTABLE
    api.flags |= 8u;
#endif
#ifdef _ITERATOR_DEBUG_LEVEL
    api.iteratorDebugLevel = _ITERATOR_DEBUG_LEVEL;
#endif
#ifdef _MSVC_LANG
    api.cppVersion = _MSVC_LANG;
#else
    api.cppVersion = __cplusplus;
#endif
    api.sharedEngine = sharedEngineIdentity();
    return api;
}

}
}
