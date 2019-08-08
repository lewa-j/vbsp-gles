#version 100
precision highp float;

varying vec2 v_uv;
varying vec4 v_clippos;
uniform sampler2D u_tex;
uniform sampler2D u_dudvtex;

void main()
{
	vec2 uv = (v_clippos.xy/v_clippos.w)*0.5+0.5;
	vec2 dudv = texture2D(u_dudvtex,v_uv).xy-0.5;
	vec4 col = texture2D(u_tex, uv+dudv*0.2);
	gl_FragColor = col*vec4(235.0/255.0, 247.0/255.0, 247.0/255.0,1.0);
}
