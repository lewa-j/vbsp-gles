//#undef BUMP
precision highp float;

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;
attribute vec4 a_lmuv;
attribute vec3 a_tangent;

varying vec3 v_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_lmuv12;
varying vec2 v_lmuv3;
varying vec3 v_tangent;

uniform mat4 u_mvpMtx;
uniform mat4 u_modelMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	v_position = (u_modelMtx * a_position).xyz;
	v_normal = normalize(mat3(u_modelMtx) * a_normal);
	v_uv = a_uv;
#ifdef BUMP
	v_lmuv12.xy = a_lmuv.xy+a_lmuv.zw;
	v_lmuv12.zw = v_lmuv12.xy+a_lmuv.zw;
	v_lmuv3 = v_lmuv12.zw+a_lmuv.zw;
#else
	v_lmuv12.xy = a_lmuv.xy;
#endif
	v_tangent = normalize(mat3(u_modelMtx) * a_tangent);
	gl_PointSize = 4.0;
}
