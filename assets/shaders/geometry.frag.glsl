#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 gbuffer_pos;
layout(location = 1) out vec4 gbuffer_norm;
layout(location = 2) out vec4 gbuffer_albedo;

layout(binding = 0) uniform sampler2D map_albedo;

void main()
{
	gbuffer_pos    = vec4(pos, 1.0);
	gbuffer_norm   = vec4(norm, 1.0);
    gbuffer_albedo = texture(map_albedo, uv);
}
