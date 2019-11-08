
#ifndef _bg2e_scene_component_hpp_
#define _bg2e_scene_component_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/path.hpp>
#include <bg2e/db/json/value.hpp>

#include <functional>
#include <unordered_map>

namespace bg2e {
namespace scene {

    class SceneObject;
    class Component : public base::ReferencedPointer {
        friend class SceneObject;
    public:
        static Component * Factory(bg2e::db::json::Value * componentData, const bg2e::base::path & resourcePath);

        Component();

        virtual Component * clone() = 0;

        inline SceneObject * sceneObject() { return _sceneObject; }
        inline const SceneObject * sceneObject() const { return _sceneObject; }
        // Node * node();
        // const Node * node() const;

        // TODO: rest of life cycle functions
        virtual void addedToNode(SceneObject *) {}
        virtual void remomvedFromNode(SceneObject *) {}

        inline void setIgnoreSerialize(bool i) { _ignoreSerialize = i; }
        inline bool ignoreSerialize() const { return _ignoreSerialize; }

        virtual void deserialize(bg2e::db::json::Value *, const bg2e::base::path &) {}
        virtual bg2e::db::json::Value * serialize(const bg2e::base::path &) { return nullptr; }


    protected:
        virtual ~Component();

        SceneObject * _sceneObject = nullptr;
        bool _ignoreSerialize = false;
    };

    class ComponentRegistry {
    public:
        static ComponentRegistry * Get() {
            if (s_singleton == nullptr) {
                s_singleton = new ComponentRegistry();
            }
            return s_singleton;
        }

        void registerFactory(const std::string & name, std::function<Component *()> factory) {
            _factoryMap[name] = factory;
        }

        Component * instantiate(const std::string & name) {
            return _factoryMap.find(name)!=_factoryMap.end() ? _factoryMap[name]() : nullptr;
        }

    protected:
        ComponentRegistry();
        virtual ~ComponentRegistry();

        static ComponentRegistry * s_singleton;
        std::unordered_map<std::string, std::function<Component*()> > _factoryMap;
    };

    template <class T>
    class ComponentFactory {
    public:
        ComponentFactory(const std::string & name) {
            ComponentRegistry::Get()->registerFactory(name, []() { return new T(); });
        }
    };

    typedef std::unordered_map<size_t, ptr<Component>> ComponentMap;
    
}
}

#endif
