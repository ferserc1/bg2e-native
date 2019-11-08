
#ifndef _bg2e_scene_scene_object_hpp_
#define _bg2e_scene_scene_object_hpp_

#include <bg2e/base/referenced_pointer.hpp>

#include <string>

namespace bg2e {
namespace scene {

    // Scene object implements the old Node functionality
    class SceneObject : public base::ReferencedPointer {
    public:
        SceneObject();
        SceneObject(const std::string & name);
        SceneObject(const char * name);

    protected:
        virtual ~SceneObject();

        std::string _name;
        bool _steady;
        bool _enabled;
    };

}
}

#endif
