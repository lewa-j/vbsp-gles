#version 100
//#define FOG 1
precision highp float;

uniform vec4 u_color;

const vec4 fogColor = vec4(50.0/255.0, 70.0/255.0, 80.0/255.0, 1.0);
//const vec4 fogColor = vec4(120.0/255.0, 155.0/255.0, 170.0/255.0, 1.0);
//const vec4 fogColor = vec4(58.0/255.0, 82.0/255.0, 101.0/255.0, 1.0);

void main()
{
	gl_FragColor = u_color;
#ifdef FOG
	gl_FragColor = mix(fogColor, gl_FragColor,clamp(1.0 /exp(abs(gl_FragCoord.z/gl_FragCoord.w) * 0.02),0.0,1.0));
#endif
}
