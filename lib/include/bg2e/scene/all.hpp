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

#include <bg2e/scene/vk/all.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/DrawVisitor.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>
#include <bg2e/scene/InputVisitor.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Mesh.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/RenderQueueVisitor.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/TransformVisitor.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/Node.hpp>
