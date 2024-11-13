
#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace ui {

class BG2E_API Input {
public:
    static void text(
        const std::string& label,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false
    );
    
    static void textWithHint(
        const std::string& label,
        const std::string& hint,
        std::string& value,
        int maxLength = 200,
        bool sameLine = false
    );
    
    static void number(
        const std::string& label,
        int * value,
        bool sameLine = false
    );
    
    static void number(
        const std::string& label,
        float * value,
        bool sameLine = false
    );
    
    static void number(
        const std::string& label,
        double * value,
        bool sameLine = false
    );
    
    static void vec2(
        const std::string& label,
        int * value,
        bool sameLine = false
    );
    
    static void vec3(
        const std::string& label,
        int * value,
        bool sameLine = false
    );
    
    static void vec4(
        const std::string& label,
        int * value,
        bool sameLine = false
    );
    
    static void vec2(
        const std::string& label,
        float * value,
        bool sameLine = false
    );
    
    static void vec3(
        const std::string& label,
        float * value,
        bool sameLine = false
    );
    
    static void vec4(
        const std::string& label,
        float * value,
        bool sameLine = false
    );
};

}
}
