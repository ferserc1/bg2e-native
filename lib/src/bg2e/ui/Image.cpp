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

#include <bg2e/ui/Image.hpp>
#include "detail/IconWidgets.hpp"

namespace bg2e::ui {

void Image::icon(const std::filesystem::path& path, float size, const IconDrawOptions& options)
{
    icon(detail::resolveIcon(path), size, options);
}

void Image::icon(const std::shared_ptr<Icon>& resource, float size, const IconDrawOptions& options)
{
    detail::drawIconImage(resource, size, options);
}

}
