#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace base {

class BG2E_API MaterialAttributes {
public:
    
    inline const Color & albedo() const { return _albedo; }
    
protected:
    
    Color _albedo;
};

}
}
