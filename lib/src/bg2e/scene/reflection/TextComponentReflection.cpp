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

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/scene/TextFontComponent.hpp>

namespace {
// Composer provides collection/paragraph transaction adapters. Registration
// exposes ordinary component membership without unsafe generic source setters.
bg2e::reflection::TypeRegistration<bg2e::scene::TextComponent> textComponent(
    [](auto& type)
    {
        type.displayName("Text");
    });
bg2e::reflection::TypeRegistration<bg2e::scene::TextFontComponent> textFontsComponent(
    [](auto& type)
    {
        type.displayName("Text Fonts");
    });
}
