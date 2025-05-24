
vec4 gammaCorrection(vec4 color, float gamma) {
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}
