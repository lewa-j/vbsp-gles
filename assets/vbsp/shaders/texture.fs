//#define FOG 1
precision highp float;

varying vec2 v_uv;
uniform sampler2D u_tex;

const vec4 fogColor = vec4(50.0/255.0, 70.0/255.0, 80.0/255.0, 1.0);
//const vec4 fogColor = vec4(40.0/255.0, 45.0/255.0, 50.0/255.0, 1.0);
//const vec4 fogColor = vec4(70.0/255.0, 85.0/255.0, 100.0/255.0, 1.0);
//const vec4 fogColor = vec4(37.0/255.0, 35.0/255.0, 33.0/255.0, 1.0);
//const vec4 fogColor = vec4(56.0/255.0, 95.0/255.0, 141.0/255.0, 1.0);
//const vec4 fogColor = vec4(120.0/255.0, 155.0/255.0, 170.0/255.0, 1.0);
//const vec4 fogColor = vec4(58.0/255.0, 82.0/255.0, 101.0/255.0, 1.0);

void main()
{
	vec4 col = texture2D(u_tex, v_uv);
#ifdef ALPHA_TEST
	if(col.a < 0.5)
		discard;
#endif
	gl_FragColor = col;
#ifdef FOG
	gl_FragColor = mix(fogColor, gl_FragColor,clamp(1.0 /exp(abs(gl_FragCoord.z/gl_FragCoord.w) * 0.02),0.0,1.0));
#endif
}
