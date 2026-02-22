$input v_color0, v_texcoord0

#include <common.sh>

void main()
{
	// SDF circle: discard fragments outside the unit circle
	float dist = dot(v_texcoord0, v_texcoord0);  // squared length — skip sqrt
	if (dist > 1.0)
		discard;

	gl_FragColor = v_color0;
}
