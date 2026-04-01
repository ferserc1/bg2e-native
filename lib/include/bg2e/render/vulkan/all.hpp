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

#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/Device.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Surface.hpp>
#include <bg2e/render/vulkan/Swapchain.hpp>

// Include vulkan namespaces
#include <bg2e/render/vulkan/factory/all.hpp>
#include <bg2e/render/vulkan/geo/all.hpp>
#include <bg2e/render/vulkan/macros/all.hpp>
