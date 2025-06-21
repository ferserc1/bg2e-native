
mat3 TBNMatrix(mat4 model, vec3 normal, vec3 tangent)
{
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(cross(tangent, normal), 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
    return mat3(T, B, N);
}


