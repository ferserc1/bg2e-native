/*
 *    business grade graphic engine (bg2e engine)
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
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/reflection/TypeInfo.hpp>
#include <string>

namespace bg2e::draw::pipeline {
// Optional final scene-color processing after tone mapping and before overlays/UI.
// One compatible component per renderer lifecycle; default renderer uses none.
// initialize/cleanup require stopped producers. record borrows a completed acquired
// slot, starts/ends outside scopes and leaves colorTarget usable for overlays.
// Retain descriptors/scratch through submission completion; do not submit here.
// settingsInstance must match settingsType, including custom multiple inheritance.
class BG2E_API FinalProcessor {
  public:
    virtual ~FinalProcessor() = default;

    virtual void initialize(Engine& engine, gpu::PixelFormat outputFormat, uint32_t slots) = 0;
    virtual bool record(const FrameContext& frame) = 0; // false leaves original color usable.
    virtual void cleanup() = 0;

    virtual bool available() const = 0;
    virtual std::string status() const = 0;
    virtual reflection::TypeInfo settingsType() const = 0;
    virtual void* settingsInstance() = 0;
};
}
