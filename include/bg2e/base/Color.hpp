#pragma once

namespace bg2e {
namespace base {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    
    Color() :r{0.0f}, g{0.0f}, b{0.0f}, a{0.0f} {}
    Color(float red, float green, float blue, float alpha = 1.0f ) :r{red}, g{green}, b{blue}, a{alpha} {}
    
    static Color Black() { return Color{ 0.0f, 0.0f, 0.0f, 1.0f }; }
    static Color White() { return Color{ 1.0f, 1.0f, 1.0f, 1.0f }; }
    static Color Red() { return Color{ 1.0f, 0.0f, 0.0f, 1.0f }; }
    static Color Green() { return Color{ 0.0f, 1.0f, 0.0f, 1.0f }; }
    static Color Blue() { return Color{ 0.0f, 0.0f, 1.0f, 1.0f }; }
    static Color Yellow() { return Color{ 1.0f, 1.0f, 0.0f, 1.0f }; }
    static Color Cyan() { return Color{ 0.0f, 1.0f, 1.0f, 1.0f }; }
    static Color Magenta() { return Color{ 1.0f, 0.0f, 1.0f, 1.0f }; }
    static Color Transparent() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; }
    static Color Gray() { return Color{ 0.5f, 0.5f, 0.5f, 1.0f }; }
    static Color LightGray() { return Color{ 0.75f, 0.75f, 0.75f, 1.0f }; }
    static Color DarkGray() { return Color{ 0.25f, 0.25f, 0.25f, 1.0f }; }
    static Color Orange() { return Color{ 1.0f, 0.5f, 0.0f, 1.0f }; }
    static Color Purple() { return Color{ 0.5f, 0.0f, 0.5f, 1.0f }; }
    static Color Pink() { return Color{ 1.0f, 0.75f, 0.8f, 1.0f }; }
    static Color Brown() { return Color{ 0.6f, 0.3f, 0.1f, 1.0f }; }
    static Color Gold() { return Color{ 1.0f, 0.84f, 0.0f, 1.0f }; }
    static Color Silver() { return Color{ 0.75f, 0.75f, 0.75f, 1.0f }; }
    static Color Bronze() { return Color{ 0.8f, 0.52f, 0.25f, 1.0f }; }
    static Color Olive() { return Color{ 0.5f, 0.5f, 0.0f, 1.0f }; }
    static Color Teal() { return Color{ 0.0f, 0.5f, 0.5f, 1.0f }; }
    static Color Navy() { return Color{ 0.0f, 0.0f, 0.5f, 1.0f }; }
    static Color Coral() { return Color{ 1.0f, 0.5f, 0.31f, 1.0f }; }
    static Color Salmon() { return Color{ 1.0f, 0.55f, 0.41f, 1.0f }; }
    static Color Lavender() { return Color{ 0.9f, 0.9f, 1.0f, 1.0f }; }
    static Color Peach() { return Color{ 1.0f, 0.8f, 0.6f, 1.0f }; }
    static Color Mint() { return Color{ 0.6f, 1.0f, 0.6f, 1.0f }; }
    static Color Indigo() { return Color{ 0.29f, 0.0f, 0.51f, 1.0f }; }
    static Color Violet() { return Color{ 0.93f, 0.51f, 0.93f, 1.0f }; }
};

}
}
