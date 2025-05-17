#pragma once

#include <bg2e/base/MaterialAttributes.hpp>

namespace bg2e {
namespace render {

class BG2E_API MaterialBase {
public:
    MaterialBase() = default;
    virtual ~MaterialBase() = default;

    void update();

    inline base::MaterialAttributes& materialAttributes() { return _materialAttributes; }
    inline const base::MaterialAttributes& materialAttributes() const { return _materialAttributes; }
    
protected:
    base::MaterialAttributes _materialAttributes;
};

}
}
