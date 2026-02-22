$input a_position, a_color0, i_data0
$output v_color0, v_texcoord0

#include <common.sh>

void main()
{
	// i_data0 = (world_x, world_y, brightness, radius)
	vec2  center     = i_data0.xy;
	float brightness = i_data0.z;
	float radius     = i_data0.w;

	// a_position.xy is the unit quad corner (-1..1)
	// Scale by radius and translate to world position
	vec2 worldPos = center + a_position.xy * radius;

	gl_Position = mul(u_viewProj, vec4(worldPos, 0.0, 1.0));

	// Pass UV for SDF circle in fragment shader (-1..1 range)
	v_texcoord0 = a_position.xy;

	// Tint color by brightness
	v_color0 = vec4(a_color0.rgb * brightness, a_color0.a);
}
