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
#include <bg2e/draw/Texture.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/app/PreferencesStore.hpp>
#include "detail/ImGuiBackend.hpp"
#include "detail/ImageRegistry.hpp"
#include "detail/ToolbarConfiguration.hpp"
#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <cmath>
#include <unordered_map>
#include <exception>
#include <stdexcept>
#include <utility>

namespace bg2e::ui {

namespace {
constexpr char uiPreferencesContext[] = "ui";
constexpr char toolbarPresentationKey[] = "toolbarPresentation";
constexpr char toolbarIconSizeKey[] = "toolbarIconSize";

ToolbarConfiguration loadToolbarConfiguration(const app::Preferences& preferences)
{
    ToolbarConfiguration configuration;
    const auto presentation = preferences.get<int32_t>(toolbarPresentationKey,
        static_cast<int32_t>(configuration.presentation));
    switch (static_cast<ToolbarPresentation>(presentation))
    {
    case ToolbarPresentation::Text:
    case ToolbarPresentation::IconsWithTooltips:
    case ToolbarPresentation::IconsAndText:
        configuration.presentation = static_cast<ToolbarPresentation>(presentation);
        break;
    default:
        // Invalid/malformed values must not prevent UI initialization.
        break;
    }
    const auto size = preferences.get(toolbarIconSizeKey, configuration.iconSize);
    if (std::isfinite(size) && size > 0.0f)
    {
        configuration.iconSize = size;
    }
    return configuration;
}

// Non-owning lookup; configuration storage belongs to UserInterface::Impl.
std::unordered_map<ImGuiContext*, ToolbarConfiguration*>& toolbarConfigurations()
{
    static std::unordered_map<ImGuiContext*, ToolbarConfiguration*> configurations;
    return configurations;
}
}

void detail::validateToolbarConfiguration(const ToolbarConfiguration& configuration)
{
    switch (configuration.presentation)
    {
    case ToolbarPresentation::Text:
    case ToolbarPresentation::IconsWithTooltips:
    case ToolbarPresentation::IconsAndText:
        break;
    default:
        throw std::invalid_argument("Unknown toolbar presentation");
    }
    if (!std::isfinite(configuration.iconSize) || configuration.iconSize <= 0.0f)
    {
        throw std::invalid_argument("Toolbar icon size must be finite and positive");
    }
}

float UserInterface::s_uiScale = 1.0f;
bool UserInterface::s_uiFontLoaded = false;
bool UserInterface::s_uiScaleChanged = true;
static bool s_baseStyleInitialized = false;
static ImGuiStyle s_baseStyle;

struct UserInterface::Impl {
    std::unique_ptr<detail::ImGuiBackend> renderer;
    std::shared_ptr<detail::ImageRegistry> images;
    std::unique_ptr<IconManager> icons;
    ToolbarConfiguration toolbarConfiguration;
    ImGuiContext* context = nullptr;
    draw::Engine* drawEngine = nullptr;
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
        if (context)
        {
            ImGui::SetCurrentContext(context);
        }
        const auto configuration = toolbarConfigurations().find(context);
        if (configuration != toolbarConfigurations().end() && configuration->second == &toolbarConfiguration)
        {
            toolbarConfigurations().erase(configuration);
        }
        icons.reset();
        if (images)
        {
            images->shutdown();
            images.reset();
        }
        if (renderer)
        {
            renderer->shutdown();
        }
        renderer.reset();
        if (sdlStarted)
        {
            ImGui_ImplSDL2_Shutdown();
        }
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

UserInterface::UserInterface() : _impl(std::make_shared<Impl>())
{
}

UserInterface::~UserInterface()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
    }
}

bool UserInterface::initialized() const
{
    return _impl->ready;
}

void UserInterface::init(draw::Engine* engine)
{
    if (!engine || !engine->surface())
    {
        throw std::invalid_argument("UI requires a windowed draw Engine.");
    }
    if (_impl->context || ImGui::GetCurrentContext())
    {
        throw std::logic_error("An ImGui context is already active.");
    }
#ifndef BG2E_IS_MAC
    if (engine->backendType() == gpu::BackendType::Metal)
    {
        throw std::invalid_argument("Metal UI is available only on macOS.");
    }
#endif
    _impl = std::make_shared<Impl>();
    _impl->drawEngine = engine;
    try
    {
        const auto& preferences = app::PreferencesStore::instance().preferences(uiPreferencesContext);
        s_uiScale = preferences.get("uiScale", s_uiScale);
        _impl->toolbarConfiguration = loadToolbarConfiguration(preferences);
        _impl->context = ImGui::CreateContext();
        if (!_impl->context)
        {
            throw std::runtime_error("ImGui context creation failed.");
        }
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
        if (!sdlInitialized)
        {
            throw std::runtime_error("ImGui SDL initialization failed.");
        }
        _impl->sdlStarted = true;
        _impl->renderer->initialize(*engine);
        _impl->images = std::make_shared<detail::ImageRegistry>(*engine, *_impl->renderer);
        _impl->icons.reset(new IconManager(*engine, _impl->images));
        toolbarConfigurations().emplace(_impl->context, &_impl->toolbarConfiguration);
        if (_delegate)
        {
            _delegate->init(engine, this);
        }
        _impl->ready = true;
    }
    catch (...)
    {
        auto error = std::current_exception();
        try
        {
            engine->device()->waitIdle();
        }
        catch (...)
        {
        }
        try
        {
            _impl->shutdown();
        }
        catch (...)
        {
        }
        std::rethrow_exception(error);
    }
}

void UserInterface::setScale(float scale)
{
    s_uiScale = scale;
    s_uiScaleChanged = true;
}

ToolbarConfiguration UserInterface::getToolbarConfiguration()
{
    const auto& configurations = toolbarConfigurations();
    const auto found = configurations.find(ImGui::GetCurrentContext());
    return found != configurations.end() ? *found->second : ToolbarConfiguration{};
}

void UserInterface::setToolbarConfiguration(const ToolbarConfiguration& configuration)
{
    detail::validateToolbarConfiguration(configuration);
    const auto& configurations = toolbarConfigurations();
    const auto found = configurations.find(ImGui::GetCurrentContext());
    if (found == configurations.end())
    {
        throw std::logic_error("Toolbar defaults require an active UI session");
    }
    const auto& previous = *found->second;
    if (previous.presentation == configuration.presentation && previous.iconSize == configuration.iconSize)
    {
        return;
    }
    *found->second = configuration;
    auto& preferences = app::PreferencesStore::instance().preferences(uiPreferencesContext);
    preferences.set(toolbarPresentationKey, static_cast<int32_t>(configuration.presentation));
    preferences.set(toolbarIconSizeKey, configuration.iconSize);
    preferences.save();
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

    auto& style = ImGui::GetStyle();
    style = s_baseStyle;

    style.ScaleAllSizes(s_uiScale);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = s_uiScale;
    s_uiScaleChanged = false;
}

bool UserInterface::textInputActive()
{
    return ImGui::GetCurrentContext() && (ImGui::GetIO().WantTextInput ||
        (ImGui::GetIO().WantCaptureKeyboard && ImGui::IsAnyItemActive()));
}


void UserInterface::processEvent(SDL_Event* event)
{
    if (!initialized() || !event)
    {
        return;
    }
    ImGui::SetCurrentContext(_impl->context);
    ImGui_ImplSDL2_ProcessEvent(event);
}

void UserInterface::setFrameOverride(std::function<void()> fn)
{
    _frameOverride = std::move(fn);
}

void UserInterface::clearFrameOverride()
{
    _frameOverride = nullptr;
}

void UserInterface::finishFrame()
{
    if (s_uiScaleChanged)
    {
        updateScale();
    }
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    if (_frameOverride)
    {
        _frameOverride();
    }
    else if (_delegate)
    {
        _delegate->drawUI();
    }
    ImGui::Render();
    _impl->prepared = true;
}

void UserInterface::newFrame(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame)
{
    if (!initialized())
    {
        return;
    }

    ImGui::SetCurrentContext(_impl->context);
    _impl->prepared = false;
    _impl->renderer->prepareFrame(&command, &frame);
    _impl->images->beginFrame();
    finishFrame();
    _impl->preparedCommand = &command;
    _impl->preparedFrame = &frame;
}

void UserInterface::draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame)
{
    if (!initialized())
    {
        return;
    }
    if (!_impl->prepared || _impl->preparedCommand != &command || _impl->preparedFrame != &frame)
    {
        throw std::logic_error("UI composition must use its prepared presentation frame and commands.");
    }
    ImGui::SetCurrentContext(_impl->context);
    _impl->renderer->draw(command, frame);
    _impl->prepared = false;
    _impl->preparedCommand = nullptr;
    _impl->preparedFrame = nullptr;
}

IconManager& UserInterface::icons()
{
    if (!_impl->icons)
    {
        throw std::logic_error("Icons require an initialized UI session");
    }
    return *_impl->icons;
}

std::shared_ptr<TexturePreview> UserInterface::texturePreview(const std::shared_ptr<draw::Texture>& texture)
{
    if (!initialized() || !_impl->images)
    {
        throw std::logic_error("Texture previews require an initialized draw UI session");
    }
    if (!texture || texture->engine() != _impl->drawEngine)
    {
        throw std::invalid_argument("Preview needs a sampleable color texture from this draw UI engine");
    }
    auto entry = _impl->images->create(texture->imageResource(), texture->samplerResource());
    return std::shared_ptr<TexturePreview>(new TexturePreview(std::move(entry)));
}

void UserInterface::cleanup()
{
    auto preferences = app::PreferencesStore::instance().preferences("ui");
    preferences.set("uiScale", s_uiScale);

    std::exception_ptr error;
    if (_impl->drawEngine)
    {
        try
        {
            _impl->drawEngine->device()->waitIdle();
        }
        catch (...)
        {
            error = std::current_exception();
        }
    }
    try
    {
        _impl->shutdown();
    }
    catch (...)
    {
        if (!error)
        {
            error = std::current_exception();
        }
    }
    _frameOverride = nullptr;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
