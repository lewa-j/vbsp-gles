precision highp float;

varying vec3 v_position;
varying vec3 v_normal;
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform vec3 u_lightPos;

void main()
{
	vec3 toLight = u_lightPos-v_position;
	float dist = length(toLight);
	float size = 15.0;
	float attenuation = 1.0 - pow( clamp(dist/size, 0.0, 1.0), 2.0);
	float light = max(0.0,dot(normalize(v_normal),normalize(toLight)));
	gl_FragColor = texture2D(u_tex,v_uv)*light*0.7*attenuation;
}