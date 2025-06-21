
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Color.hpp>

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
    
    static void slider(
        const std::string& label,
        int * value,
        int min = 0,
        int max = 100,
        bool sameLine = false
    );
    
    static void slider(
        const std::string& label,
        float * value,
        float min = 0.0f,
        float max = 1.0f,
        bool sameLine = false
    );
    
    static void colorPicker(
        const std::string& label,
        bg2e::base::Color & color,
        bool sameLine = false
    );
};

}
}
