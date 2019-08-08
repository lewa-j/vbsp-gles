#version 100
precision highp float;

attribute vec4 a_position;
attribute float a_alpha;
attribute vec2 a_uv;
attribute vec2 a_lmuv;

varying float v_alpha;
varying vec2 v_uv;
varying vec2 v_lmuv;

uniform mat4 u_mvpMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	v_alpha = a_alpha;
	v_uv = a_uv;
	v_lmuv = a_lmuv;
	gl_PointSize = 4.0;
}
