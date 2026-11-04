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

#include <bg2e/ui/Icon.hpp>
#include "detail/IconData.hpp"
#include "detail/ImageDraw.hpp"
#include <utility>

namespace bg2e::ui {

Icon::Icon(std::shared_ptr<detail::IconData> data) : _data(std::move(data))
{
}

bool Icon::valid() const
{
    return _data && !_data->session.expired() && detail::imageValid(_data->image);
}

bool Icon::isFallback() const
{
    return _data && _data->fallback;
}

const std::filesystem::path& Icon::source() const
{
    return _data->source;
}

}
