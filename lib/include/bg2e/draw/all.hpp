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

#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/draw/RenderLoop.hpp>

#include <bg2e/draw/Renderer.hpp>
#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <bg2e/draw/environment/all.hpp>

#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/DefaultOffscreenApplicationDelegate.hpp>

#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/Texture.hpp>
#include <bg2e/draw/MaterialBase.hpp>

#include <bg2e/draw/ShaderData.hpp>
#include <bg2e/draw/RenderQueue.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/draw/DrawVisitor.hpp>
#include <bg2e/draw/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/draw/FrameDataBinding.hpp>
#include <bg2e/draw/ObjectDataBinding.hpp>
#include <bg2e/draw/LightDataBinding.hpp>
#include <bg2e/draw/EnvironmentDataBinding.hpp>

#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/draw/pipeline/FullscreenPass.hpp>
#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <bg2e/draw/pipeline/DeferredLayer.hpp>

#include <bg2e/draw/pipeline/SoftShadowPass.hpp>

#include <bg2e/draw/RenderSettingsPreferences.hpp>

#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/draw/FsrUpscaler.hpp>

#include <bg2e/draw/pipeline/OverlayPass.hpp>

#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>

#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>

#include <bg2e/draw/pipeline/RTReflections.hpp>

#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/draw/pipeline/MotionVectorGenerator.hpp>

#include <bg2e/draw/pipeline/DenoiseFilter.hpp>

#include <bg2e/draw/pipeline/SMAAProcessor.hpp>

#include <bg2e/draw/pipeline/FrameAccumulator.hpp>
