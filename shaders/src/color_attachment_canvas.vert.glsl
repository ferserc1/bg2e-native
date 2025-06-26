#version 450

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPosition;

void main()
{
    const vec3 positions[4] = vec3[4](
        vec3(-1.0, -1.0, 0.0),
        vec3( 1.0, -1.0, 0.0),
        vec3( 1.0,  1.0, 0.0),
        vec3(-1.0,  1.0, 0.0)
    );
    
    const vec2 uvs[4] = vec2[4](
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0)
    );
    
    const int indexes[6] = {
        0, 1, 2,
        2, 3, 0
    };
    
    int index = indexes[gl_VertexIndex];
    outPosition = positions[index];
    gl_Position = vec4(outPosition, 1.0f);
    outUV = uvs[index];
}
