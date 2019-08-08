precision highp float;

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;

varying vec3 v_position;
varying vec2 v_uv;
varying float v_light;
varying vec3 v_normal;

uniform mat4 u_mvpMtx;
uniform mat4 u_modelMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	v_position = (u_modelMtx * a_position).xyz;
	v_normal = normalize(mat3(u_modelMtx) * a_normal);
	v_light = max(0.3,dot(v_normal,normalize(vec3(0.5,0.5,1.0))));
	v_uv = a_uv;
	gl_PointSize = 4.0;
}
