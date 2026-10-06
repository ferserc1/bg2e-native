#include <bg2e/render/BakerContext.hpp>

#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Node.hpp>

#include <stdexcept>

namespace bg2e {
namespace render {

BakerContext::BakerContext(Engine* engine, scene::Node* rootNode)
    : _engine(engine), _rootNode(rootNode)
{
    if (!_engine) {
        throw std::invalid_argument("BakerContext: engine must not be null");
    }
    if (!_rootNode) {
        throw std::invalid_argument("BakerContext: rootNode must not be null");
    }
    if (!_engine->rayTracingSupported()) {
        throw std::runtime_error("BakerContext requires ray tracing support");
    }
}

}
}
