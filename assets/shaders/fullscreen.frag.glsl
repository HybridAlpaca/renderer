#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform sampler2D rendered_image;

void main()
{
	const vec2 uv = { uv_coords.s, uv_coords.t };

	frag_color = texture(rendered_image, uv);
}
