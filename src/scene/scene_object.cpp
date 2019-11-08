
#include <bg2e/scene/scene_object.hpp>

namespace bg2e {
namespace scene {

    SceneObject::SceneObject()
        :_name()
    {

    }

    SceneObject::SceneObject(const std::string & name)
        :_name(name)
    {

    }

    SceneObject::SceneObject(const char * name)
        :_name(name)
    {

    }

    SceneObject::~SceneObject() {

    }

}
}
