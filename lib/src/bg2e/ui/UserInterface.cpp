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

#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/app/PreferencesStore.hpp>
#include "detail/ImGuiBackend.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include <exception>
#include <stdexcept>
#include <utility>

namespace bg2e::ui {

float UserInterface::s_uiScale = 1.0f;
bool UserInterface::s_uiFontLoaded = false;
bool UserInterface::s_uiScaleChanged = true;
static bool s_baseStyleInitialized = false;
static ImGuiStyle s_baseStyle;

struct UserInterface::Impl {
    std::unique_ptr<detail::ImGuiBackend> renderer;
    ImGuiContext* context = nullptr;
    draw::Engine* drawEngine = nullptr;
    bool production = false;
    bool sdlStarted = false;
    bool ready = false;
    bool prepared = false;
    gpu::CommandBuffer* preparedCommand = nullptr;
    gpu::SurfaceFrame* preparedFrame = nullptr;

    void shutdown()
    {
        ready = false;
        prepared = false;
        preparedCommand = nullptr;
        preparedFrame = nullptr;
        if (context) ImGui::SetCurrentContext(context);
        if (renderer) renderer->shutdown();
        renderer.reset();
        if (sdlStarted) ImGui_ImplSDL2_Shutdown();
        sdlStarted = false;
        if (context)
        {
            ImGui::DestroyContext(context);
            s_uiFontLoaded = false;
            s_uiScaleChanged = true;
            s_baseStyleInitialized = false;
        }
        context = nullptr;
        drawEngine = nullptr;
    }
};

UserInterface::UserInterface() : _impl(std::make_shared<Impl>()) {}
UserInterface::~UserInterface()
{
    try { cleanup(); } catch (...) { }
}

bool UserInterface::initialized() const { return _impl->ready; }

void UserInterface::init(render::Engine* engine)
{
    if (!engine) throw std::invalid_argument("UI requires a production Engine.");
    if (_impl->context || ImGui::GetCurrentContext()) throw std::logic_error("An ImGui context is already active.");
    _impl = std::make_shared<Impl>(); // Old engine callbacks retain only their own lifecycle.
    _engine = engine;
    _impl->production = true;
    // Capture retained state, never this: the engine determines production
    // shutdown ordering even if the UI wrapper is destroyed first.
    engine->cleanupManager().push([state = _impl](VkDevice) { state->shutdown(); });
    try
    {
        s_uiScale = app::PreferencesStore::instance().preferences("ui").get("uiScale", s_uiScale);
        _impl->context = ImGui::CreateContext();
        if (!_impl->context) throw std::runtime_error("ImGui context creation failed.");
        if (!ImGui_ImplSDL2_InitForVulkan(static_cast<SDL_Window*>(engine->windowPtr())))
            throw std::runtime_error("ImGui SDL Vulkan initialization failed.");
        _impl->sdlStarted = true;
        _impl->renderer = detail::createVulkanImGuiBackend();
        _impl->renderer->initialize(*engine);
        if (_delegate) _delegate->init(engine, this);
        _impl->ready = true;
    }
    catch (...)
    {
        auto error = std::current_exception();
        try { engine->device().waitIdle(); } catch (...) { }
        try { _impl->shutdown(); } catch (...) { }
        std::rethrow_exception(error);
    }
}

void UserInterface::init(draw::Engine* engine)
{
    if (!engine) throw std::invalid_argument("UI requires a draw Engine.");
    if (_impl->context || ImGui::GetCurrentContext()) throw std::logic_error("An ImGui context is already active.");
#ifndef BG2E_IS_MAC
    if (engine->backendType() == gpu::BackendType::Metal)
        throw std::invalid_argument("Metal UI is available only on macOS.");
#endif
    _impl = std::make_shared<Impl>();
    _impl->production = false;
    _impl->drawEngine = engine;
    _engine = nullptr;
    try
    {
        s_uiScale = app::PreferencesStore::instance().preferences("ui").get("uiScale", s_uiScale);
        _impl->context = ImGui::CreateContext();
        if (!_impl->context) throw std::runtime_error("ImGui context creation failed.");
        bool sdlInitialized = false;
        switch (engine->backendType())
        {
        case gpu::BackendType::Vulkan:
            _impl->renderer = detail::createVulkanImGuiBackend();
            sdlInitialized = ImGui_ImplSDL2_InitForVulkan(engine->instance()->window());
            break;
        case gpu::BackendType::Metal:
#ifdef BG2E_IS_MAC
            _impl->renderer = detail::createMetalImGuiBackend();
            sdlInitialized = ImGui_ImplSDL2_InitForMetal(engine->instance()->window());
            break;
#else
            throw std::invalid_argument("Metal UI is available only on macOS.");
#endif
        default:
            throw std::invalid_argument("Unsupported draw UI backend.");
        }
        if (!sdlInitialized) throw std::runtime_error("ImGui SDL initialization failed.");
        _impl->sdlStarted = true;
        _impl->renderer->initialize(*engine);
        if (_delegate) _delegate->init(engine, this);
        _impl->ready = true;
    }
    catch (...)
    {
        auto error = std::current_exception();
        try { engine->device()->waitIdle(); } catch (...) { }
        try { _impl->shutdown(); } catch (...) { }
        std::rethrow_exception(error);
    }
}

void UserInterface::setScale(float scale)
{
    s_uiScale = scale;
    s_uiScaleChanged = true;
}

void UserInterface::updateScale()
{
    if (!s_baseStyleInitialized)
    {
        s_baseStyle = ImGui::GetStyle();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        auto assets = base::PlatformTools::assetPath() / "DidactGothic-Regular.ttf";
        io.Fonts->AddFontFromFileTTF(assets.string().c_str(), 16.0f);
        io.Fonts->Build();
        s_uiFontLoaded = true;
        s_baseStyleInitialized = true;
    }

    auto & style = ImGui::GetStyle();
    style = s_baseStyle;

    style.ScaleAllSizes(s_uiScale);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = s_uiScale;
    s_uiScaleChanged = false;
}

void UserInterface::processEvent(SDL_Event* event)
{
    if (!initialized() || !event) return;
    ImGui::SetCurrentContext(_impl->context);
    ImGui_ImplSDL2_ProcessEvent(event);
}

void UserInterface::setFrameOverride(std::function<void()> fn) { _frameOverride = std::move(fn); }
void UserInterface::clearFrameOverride() { _frameOverride = nullptr; }

void UserInterface::finishFrame()
{
    if (s_uiScaleChanged) updateScale();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    if (_frameOverride) _frameOverride();
    else if (_delegate) _delegate->drawUI();
    ImGui::Render();
    _impl->prepared = true;
}

void UserInterface::newFrame()
{
    if (!initialized()) return;
    if (!_impl->production) throw std::logic_error("Draw UI newFrame requires an acquired frame context.");
    ImGui::SetCurrentContext(_impl->context);
    _impl->renderer->prepareFrame(nullptr, nullptr);
    finishFrame();
}

void UserInterface::newFrame(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame)
{
    if (!initialized()) return;
    if (_impl->production) throw std::logic_error("Abstract UI frames require a draw Engine.");
    ImGui::SetCurrentContext(_impl->context);
    _impl->prepared = false;
    _impl->renderer->prepareFrame(&command, &frame);
    finishFrame();
    _impl->preparedCommand = &command;
    _impl->preparedFrame = &frame;
}

void UserInterface::draw(VkCommandBuffer command, VkImageView imageView)
{
    if (!initialized() || !_impl->prepared) return;
    ImGui::SetCurrentContext(_impl->context);
    _impl->renderer->draw(command, imageView);
    _impl->prepared = false;
}

void UserInterface::draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame)
{
    if (!initialized()) return;
    if (!_impl->prepared || _impl->preparedCommand != &command || _impl->preparedFrame != &frame)
        throw std::logic_error("UI composition must use its prepared presentation frame and commands.");
    ImGui::SetCurrentContext(_impl->context);
    _impl->renderer->draw(command, frame);
    _impl->prepared = false;
    _impl->preparedCommand = nullptr;
    _impl->preparedFrame = nullptr;
}

void UserInterface::cleanup()
{
    auto preferences = app::PreferencesStore::instance().preferences("ui");
    preferences.set("uiScale", s_uiScale);
    if (_impl->production) return; // The production engine owns registered shutdown.
    std::exception_ptr error;
    if (_impl->drawEngine)
    {
        try { _impl->drawEngine->device()->waitIdle(); } catch (...) { error = std::current_exception(); }
    }
    try { _impl->shutdown(); } catch (...) { if (!error) error = std::current_exception(); }
    _frameOverride = nullptr;
    if (error) std::rethrow_exception(error);
}

}
