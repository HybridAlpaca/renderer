#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_norm;
layout(location = 2) out vec2 out_uv;

layout(push_constant) uniform Matrices
{
	mat4 model;
	mat4 view;
	mat4 proj;
};

void main()
{
	out_pos  = pos;
	out_norm = norm;
	out_uv   = uv;

    gl_Position = proj * view * model * vec4(pos, 1.0);
}
