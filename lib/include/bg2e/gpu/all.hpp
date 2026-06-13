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

#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/gpu/FrameResourceRing.hpp>
#include <bg2e/gpu/Backend.hpp>
#include <bg2e/gpu/Factory.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/OffscreenSurface.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/ShaderModule.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/Mesh.hpp>
