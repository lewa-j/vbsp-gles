
precision highp float;

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;

varying vec2 v_uv;
varying vec3 v_light;

uniform mat4 u_mvpMtx;
uniform mat4 u_modelMtx;
uniform vec3 u_ambientCube[6];

void main()
{
	gl_Position = u_mvpMtx * a_position;
	vec3 n_normal = normalize(mat3(u_modelMtx) * a_normal);
	//vec3 n_normal = normalize(a_normal);
	v_uv = a_uv;
	gl_PointSize = 4.0;
	
	vec3 nSquared = n_normal * n_normal;
	//ivec3 isNegative = ( n_normal < vec3(0.0) );
	ivec3 isNegative = ivec3(n_normal.x<0.0, n_normal.y<0.0, n_normal.z<0.0);
	
	v_light = nSquared.x * u_ambientCube[isNegative.x] +
	              nSquared.y * u_ambientCube[isNegative.y+2] +
	              nSquared.z * u_ambientCube[isNegative.z+4];
	
	v_light*=4.0;
	//v_light=1.0;
}
