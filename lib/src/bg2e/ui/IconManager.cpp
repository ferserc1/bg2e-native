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

#include <bg2e/ui/IconManager.hpp>
#include <bg2e/base/Image.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/draw/Texture.hpp>
#include "detail/IconAccess.hpp"
#include "detail/IconData.hpp"
#include "detail/ImageRegistry.hpp"
#include <imgui.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace bg2e::ui {
namespace {
// UI-thread-only context lookup. Cache/resource ownership remains in the UI.
std::unordered_map<ImGuiContext*, IconManager*>& activeManagers()
{
    static std::unordered_map<ImGuiContext*, IconManager*> managers;
    return managers;
}

std::unique_ptr<base::Image> questionMarkPixels()
{
    constexpr uint32_t width = 16;
    constexpr uint32_t height = 16;
    constexpr const char* rows[] = {
        "0000000000000000",
        "0000000000000000",
        "0000011111100000",
        "0000111111110000",
        "0001110000111000",
        "0001100000011000",
        "0000000000111000",
        "0000000001110000",
        "0000000011100000",
        "0000000111000000",
        "0000000110000000",
        "0000000000000000",
        "0000000110000000",
        "0000000110000000",
        "0000000000000000",
        "0000000000000000"
    };
    auto data = std::make_unique<unsigned char[]>(width * height * 4);
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            const auto offset = (y * width + x) * 4;
            data[offset] = data[offset + 1] = data[offset + 2] = 255;
            data[offset + 3] = rows[y][x] == '1' ? 255 : 0;
        }
    }
    auto pixels = std::make_unique<base::Image>();
    pixels->setData(data.release(), width, height, 4);
    return pixels;
}
}

struct IconManager::Impl {
    draw::Engine* engine;
    std::shared_ptr<detail::ImageRegistry> images;
    ImGuiContext* context;
    std::shared_ptr<detail::IconSession> session = std::make_shared<detail::IconSession>();
    std::unordered_map<std::filesystem::path, std::shared_ptr<Icon>> cache;
    std::shared_ptr<detail::ImageEntry> fallback;
    bool fallbackAttempted = false;

    std::shared_ptr<detail::ImageEntry> upload(const base::Image& pixels)
    {
        base::Texture settings;
        // Pixels are uploaded directly. A nonempty source label prevents the
        // texture's empty-path reload contract from retaining a CPU pixel copy.
        // This temporary texture is never reloaded; Icon owns the real path.
        settings.setImageFilePath("ui-icon-pixels");
        settings.setUseMipmaps(false);
        settings.setMinFilter(base::Texture::FilterLinear);
        settings.setMagFilter(base::Texture::FilterLinear);
        settings.setAddressMode(base::Texture::AddressModeClampToEdge);
        // ImGui multiplies sampled UNORM bytes by UI colors and uses straight
        // alpha blending. Preserve authored PNG bytes, with no material gamma
        // conversion or premultiplication. Texture uploads use RGBA8_UNORM.
        settings.setColorType(base::Color::TypeLinear);
        draw::Texture texture(engine);
        texture.load(pixels, settings);
        return images->create(texture.imageResource(), texture.samplerResource());
    }

    std::shared_ptr<detail::ImageEntry> fallbackImage()
    {
        if (!fallbackAttempted)
        {
            fallbackAttempted = true;
            // A fallback upload is best-effort. Normal icon uploads below are
            // deliberately outside the asset-error catch and still propagate.
            try
            {
                auto pixels = questionMarkPixels();
                fallback = upload(*pixels);
            }
            catch (const std::runtime_error& error)
            {
                std::clog << "UI icon fallback unavailable: " << error.what() << '\n';
            }
            catch (const std::logic_error& error)
            {
                std::clog << "UI icon fallback unavailable: " << error.what() << '\n';
            }
        }
        return fallback;
    }
};

IconManager::IconManager(draw::Engine& engine, std::shared_ptr<detail::ImageRegistry> images)
    : _impl(std::make_unique<Impl>())
{
    _impl->engine = &engine;
    _impl->images = std::move(images);
    _impl->context = ImGui::GetCurrentContext();
    if (!_impl->context || !_impl->images)
    {
        throw std::logic_error("IconManager requires an active UI session");
    }
    if (!activeManagers().emplace(_impl->context, this).second)
    {
        throw std::logic_error("This UI context already has an icon manager");
    }
}

IconManager::~IconManager()
{
    auto& managers = activeManagers();
    const auto found = managers.find(_impl->context);
    if (found != managers.end() && found->second == this)
    {
        managers.erase(found);
    }
    // Invalidate even fallback handles without an image before registry shutdown.
    _impl->session.reset();
}

std::shared_ptr<Icon> IconManager::load(const std::filesystem::path& path)
{
    if (ImGui::GetCurrentContext() != _impl->context)
    {
        throw std::logic_error("Icon loading requires its owning UI context");
    }
    if (path.empty())
    {
        return nullptr;
    }
    std::error_code pathError;
    auto source = std::filesystem::absolute(path, pathError);
    // Unresolvable paths are also cached, under their supplied lexical form.
    // Relative failure keys cannot collide with successful absolute source keys.
    source = (pathError ? path : source).lexically_normal();
    if (const auto found = _impl->cache.find(source); found != _impl->cache.end())
    {
        return found->second;
    }

    std::unique_ptr<base::Image> pixels;
    std::string error;
    if (pathError)
    {
        error = pathError.message();
    }
    else
    {
        // This catch covers only filesystem/decoder failures. GPU allocation,
        // upload and registration errors must not become cached missing assets.
        try
        {
            if (db::imageFormatFromPath(source) != db::ImageFormat::PNG)
            {
                error = "UI icons require a PNG source";
            }
            else
            {
                pixels.reset(db::loadImage(source));
                if (!pixels || !pixels->data() || pixels->width() == 0 ||
                    pixels->height() == 0 || pixels->channels() != 4)
                {
                    error = "PNG decoder did not return RGBA8 pixels";
                }
            }
        }
        catch (const std::runtime_error& failure)
        {
            error = failure.what();
        }
    }

    auto data = std::make_shared<detail::IconData>();
    data->source = source;
    data->session = _impl->session;
    data->fallback = !error.empty();
    data->image = data->fallback ? _impl->fallbackImage() : _impl->upload(*pixels);
    auto icon = std::shared_ptr<Icon>(new Icon(std::move(data)));
    _impl->cache.emplace(source, icon);
    if (icon->isFallback())
    {
        std::clog << "Unable to load UI icon ";
        try
        {
            std::clog << source;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // A native path can be a valid cache key even when conversion to
            // the narrow diagnostic stream is unavailable on this platform.
            std::clog << "<unprintable native path>";
        }
        std::clog << ": " << error << '\n';
    }
    return icon;
}

namespace detail {

IconManager* IconAccess::current()
{
    const auto& managers = activeManagers();
    const auto found = managers.find(ImGui::GetCurrentContext());
    return found != managers.end() ? found->second : nullptr;
}

bool IconAccess::belongsToCurrentSession(const Icon& icon)
{
    const auto* manager = current();
    return manager && icon._data && icon._data->session.lock() == manager->_impl->session;
}

std::shared_ptr<ImageEntry> IconAccess::image(const Icon& icon)
{
    return belongsToCurrentSession(icon) ? icon._data->image : nullptr;
}

}
}
