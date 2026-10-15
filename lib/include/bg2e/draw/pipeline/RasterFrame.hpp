#pragma once

#include <bg2e/draw/FrameDataBinding.hpp>
#include <bg2e/draw/ObjectDataBinding.hpp>
#include <bg2e/draw/LightDataBinding.hpp>
#include <bg2e/draw/EnvironmentDataBinding.hpp>
#include <bg2e/draw/RenderQueue.hpp>

namespace bg2e::draw::pipeline {
// All pointers are borrowed for one pass. Null item selects the opaque queue;
// non-null item selects exactly one sorted transparent submesh.
struct RasterFrame {
    const RenderQueue* queue = nullptr;
    const RenderQueue::Item* item = nullptr;
    FrameDataBinding* frame = nullptr;
    ObjectDataBinding* object = nullptr;
    LightDataBinding* lights = nullptr;
    EnvironmentDataBinding* environment = nullptr;
};
}
