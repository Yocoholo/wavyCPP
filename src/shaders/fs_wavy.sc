$input v_color0

#include <common.sh>

uniform vec4 u_brightness;

void main()
{
	float brightness = u_brightness.x;
	brightness = (brightness)/2;
	gl_FragColor = vec4(v_color0.x*brightness, v_color0.y*brightness, v_color0.z*brightness, v_color0.w*brightness);
}
