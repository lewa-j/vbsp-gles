#version 100
#define FOG 1
precision highp float;

varying float v_alpha;
varying vec2 v_uv;
varying vec2 v_lmuv;

uniform sampler2D u_tex;
uniform sampler2D u_lmtex;
uniform sampler2D u_tex2;

//const vec4 fogColor = vec4(58.0/255.0, 82.0/255.0, 101.0/255.0, 1.0;
//const vec4 fogColor = vec4(40.0/255.0, 45.0/255.0, 50.0/255.0, 1.0);
//const vec4 fogColor = vec4(21.0/255.0, 23.0/255.0, 19.0/255.0, 1.0); //d2_coast_09
const vec4 fogColor = vec4(140.0/255.0, 157.0/255.0, 176.0/255.0, 1.0); //d2_coast_09

void main()
{
	//if(v_alpha>1.0)
	//	discard;
	gl_FragColor = mix(texture2D(u_tex, v_uv),texture2D(u_tex2, v_uv),pow(clamp(1.0,0.0,v_alpha),2.2)) * texture2D(u_lmtex, v_lmuv) * 2.0;
	//gl_FragColor = mix(texture2D(u_tex, v_uv),texture2D(u_tex2, v_uv),clamp(1.0,0.0,v_alpha));
	//gl_FragColor = vec4(v_alpha);
	//gl_FragColor = texture2D(u_lmtex, v_lmuv)*2.0;

#ifdef FOG
	gl_FragColor = mix(fogColor, gl_FragColor, clamp(1.0 /exp(abs(gl_FragCoord.z/gl_FragCoord.w) * 0.02), 0.0, 1.0));
#endif
}
