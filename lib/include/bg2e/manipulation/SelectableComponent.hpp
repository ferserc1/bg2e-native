
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/Component.hpp>

#include <unordered_map>

namespace bg2e {
namespace manipulation {

class BG2E_API SelectableComponent : public scene::Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Selectable");
    
    SelectableComponent();
    virtual ~SelectableComponent() = default;
    
    void update(float) override;

    inline uint32_t identifier(uint32_t submeshIndex) { return _identifier[submeshIndex]; }
    inline uint32_t submeshCount() const { return _submeshCount; }
    
    static inline uint32_t decodeObjectId(const uint8_t data[4])
    {
        return
            static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);
    }
    
protected:
    // submeshIndex, submeshIdentifier
    std::unordered_map<uint32_t, uint32_t> _identifier;
    uint32_t _submeshCount = 0;
    
    static uint32_t _lastIdentifier;
    static uint32_t generateIdentifier();
};

}
}
