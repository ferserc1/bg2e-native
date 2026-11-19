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

#include <bg2e/db/drawable_cache.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/draw/Engine.hpp>

#include <exception>
#include <utility>

namespace bg2e::db {

draw::DrawableCacheResult loadCachedDrawableBg2(const std::filesystem::path& path,
    draw::Engine& engine, std::function<void()> onTextureLoaded)
try
{
    return loadCachedDrawableBg2(path, engine.drawableCache(), std::move(onTextureLoaded));
}
catch (const std::exception& error)
{
    return {{}, {draw::DrawableCacheCode::LoadFailed, error.what()}};
}
catch (...)
{
    return {{}, {draw::DrawableCacheCode::LoadFailed, "Cannot access the engine drawable cache"}};
}


draw::DrawableCacheResult loadCachedDrawableBg2(const std::filesystem::path& path,
    draw::DrawableCache& cache, std::function<void()> onTextureLoaded)
try
{
    return cache.load(path, [onTextureLoaded](const auto& source, auto& context)
    {
        return loadDrawableBg2(source, context, onTextureLoaded);
    });
}
catch (const std::exception& error)
{
    return {{}, {draw::DrawableCacheCode::LoadFailed, error.what()}};
}
catch (...)
{
    return {{}, {draw::DrawableCacheCode::LoadFailed, "Cannot access the engine drawable cache"}};
}

}
