#version 100
precision highp float;

attribute vec4 a_position;
attribute vec2 a_uv;

varying vec4 v_clippos;
varying vec2 v_uv;

uniform mat4 u_mvpMtx;
uniform mat4 u_modelMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	v_clippos = gl_Position;
	v_uv = a_uv;
	gl_PointSize = 4.0;
}
