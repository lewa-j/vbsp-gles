
#define FOG 1
precision highp float;

varying vec2 v_uv;
varying vec3 v_light;

uniform sampler2D u_tex;
//uniform vec3 u_ambientLight;

//const vec4 fogColor = vec4(58.0/255.0, 82.0/255.0, 101.0/255.0, 1.0);
//const vec4 fogColor = vec4(40.0/255.0, 45.0/255.0, 50.0/255.0, 1.0);
//const vec4 fogColor = vec4(21.0/255.0, 23.0/255.0, 19.0/255.0, 1.0); //d2_coast_09
const vec4 fogColor = vec4(140.0/255.0, 157.0/255.0, 176.0/255.0, 1.0); //d2_coast_09

void main()
{
	vec4 col = texture2D(u_tex, v_uv);
#ifdef ALPHA_TEST
	if(col.a < 0.5)
		discard;
#endif

#ifdef AMBIENT_LIGHT
	col.rgb *= v_light;
#endif
	gl_FragColor = col;
	//gl_FragColor.rgb  = v_light*0.5;
#ifdef FOG
	gl_FragColor = mix(fogColor, gl_FragColor,clamp(1.0 /exp(abs(gl_FragCoord.z/gl_FragCoord.w) * 0.02),0.0,1.0));
#endif
}
