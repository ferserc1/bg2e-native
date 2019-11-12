
#ifndef _bg2e_scene_component_hpp_
#define _bg2e_scene_component_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/path.hpp>
#include <bg2e/db/json/value.hpp>

#include <functional>
#include <unordered_map>

namespace bg2e {
namespace scene {

    class Node;
    class Component : public base::ReferencedPointer {
        friend class Node;
    public:
        static Component * Factory(bg2e::db::json::Value * componentData, const bg2e::base::path & resourcePath);

        Component();

        virtual Component * clone() = 0;

        inline Node * node() { return _node; }
        inline const Node * node() const { return _node; }

        // TODO: rest of life cycle functions
        virtual void addedToNode(Node *) {}
        virtual void removedFromNode(Node *) {}

        inline void setIgnoreSerialize(bool i) { _ignoreSerialize = i; }
        inline bool ignoreSerialize() const { return _ignoreSerialize; }

        virtual void deserialize(bg2e::db::json::Value *, const bg2e::base::path &) {}
        virtual bg2e::db::json::Value * serialize(const bg2e::base::path &) { return nullptr; }


    protected:
        virtual ~Component();

        Node * _node = nullptr;
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
	typedef std::vector<ptr<Component>> ComponentVector;
	typedef std::vector<Component*> ComponentVectorWeak;
    
}
}

#endif
