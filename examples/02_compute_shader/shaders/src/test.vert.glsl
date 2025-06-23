#version 450

layout(location = 0) out vec2 uv;

void main() {
    const vec3 positions[4] = vec3[4](
		vec3( 0.3f, 0.3f, 0.0f),
		vec3(-0.3f, 0.3f, 0.0f),
		vec3(-0.3f,-0.3f, 0.0f),
        vec3( 0.3f,-0.3f, 0.0f)
	);

	const vec2 uvs[4] = vec2[4](
		vec2(1.0f, 1.0f),
		vec2(0.0f, 1.0f),
		vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f)
	);

	const int indices[6] = int[6](
		0, 1, 2,
		2, 3, 0
	);

	int index = indices[gl_VertexIndex];
	vec3 position = positions[index];
	gl_Position = vec4(position, 1.0f);
	uv = uvs[index];
}
