
mat3 TBNMatrix(mat4 model, vec3 normal, vec3 tangent)
{
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(cross(tangent, normal), 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
    return mat3(T, B, N);
}


vec3 sampleNormal(sampler2D normalMap, vec2 texCoord)
{
    vec3 normal = texture(normalMap, texCoord).xyz * 2.0 - 1.0;
    return normalize(normal);
}

