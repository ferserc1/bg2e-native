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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/render/ColorAttachmentsCanvas.hpp>
#include <bg2e/render/CubemapRenderer.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/DefaultOffscreenApplicationDelegate.hpp>
#include <bg2e/render/DefaultRenderLoopDelegate.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/IrradianceCubemapRenderer.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/Renderer.hpp>
#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/RenderSettingsPreferences.hpp>
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/render/RenderLoopDelegate.hpp>
#include <bg2e/render/RenderQueue.hpp>
#include <bg2e/render/SkyboxRenderer.hpp>
#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>
#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/Texture.hpp>

#include <bg2e/render/vulkan/all.hpp>
#include <bg2e/render/uniforms/all.hpp>
