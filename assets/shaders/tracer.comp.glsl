#version 450

#extension GL_ARB_separate_shader_objects : enable



layout (local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0) uniform sampler2D gbuffer_pos;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_norm;
layout(set = 0, binding = 2) uniform sampler2D gbuffer_albedo;

layout(set = 0, binding = 3, rgba8) uniform writeonly image2D render_target;



void main()
{
	// Compute camera coordinates and ray direction

	const ivec2 render_target_size = imageSize(render_target);

	const vec2 uv = vec2(gl_GlobalInvocationID.xy) / render_target_size;

	const vec4 raw_pos  = texture(gbuffer_pos, uv);
	const vec4 raw_norm = texture(gbuffer_norm, uv);
	const vec4 albedo   = texture(gbuffer_albedo, uv);

	vec4 color = {0.0, 0.0, 0.0, 1.0};

	// `w` value of gbuffer.pos is obly guaranteed to be one if
	// a fragment resides there.  Therefore, if `w` is not equal
	// to one, there is no object in front of the background and
	// we may skip shading.

	if (raw_pos.w == 1.0)
	{
		const vec3 pos  = raw_pos.xyz;
		const vec3 norm = raw_norm.xyz;

		const vec3 lightPos = {3.0, 3.0, -3.0};

		vec3 lightDir = normalize(lightPos - pos);
		float diffuse = max(dot(norm, lightDir), 0.0);

		color = diffuse * albedo;
	}

	imageStore(render_target, ivec2(gl_GlobalInvocationID.xy), color);
}
