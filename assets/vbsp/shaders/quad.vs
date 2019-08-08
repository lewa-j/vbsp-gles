#version 100
precision highp float;

attribute vec4 a_position;
varying vec2 v_uv;
uniform vec4 u_transform;

void main()
{
	gl_Position = a_position;
	gl_Position.xy = a_position.xy*u_transform.zw+u_transform.xy;
	v_uv = a_position.xy*0.5+0.5;
}
