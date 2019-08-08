precision highp float;

varying float v_light;
varying vec3 v_position;
varying vec2 v_uv;
varying vec3 v_normal;

uniform sampler2D u_tex;

void main()
{
	vec4 col = texture2D(u_tex, v_uv);
	float p_light = max(0.3,dot(normalize(v_normal),normalize(vec3(0.5,0.5,1.0))));
	gl_FragColor = col*vec4(vec3(p_light),1.0);
	//gl_FragColor = col*vec4(vec3(v_light),1.0);
}
