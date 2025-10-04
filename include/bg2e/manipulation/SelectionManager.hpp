//
//  SelectionManager.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace manipulation {

class SelectionItem {
    std::shared_ptr<scene::Drawable> mesh;
    uint32_t submesh;
};

class BG2E_API SelectionManager {
public:
    SelectionManager(render::Engine * engine);
    virtual ~SelectionManager() = default;

protected:
    render::Engine * _engine;
};

}
}
