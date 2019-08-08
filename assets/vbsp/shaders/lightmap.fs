
#define FOG 1
//#undef BUMP
//#define ENVMAP 0

precision highp float;

varying vec3 v_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_lmuv12;
varying vec2 v_lmuv3;
varying vec3 v_tangent;

uniform sampler2D u_tex;
uniform sampler2D u_lmtex;
uniform samplerCube u_envmap;
uniform sampler2D u_bumpmap;
uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_cameraPos;
uniform vec3 u_envmapTint;
const float overbright = 1.0;

//const vec4 fogColor = vec4(50.0/255.0, 70.0/255.0, 80.0/255.0, 1.0);
//const vec4 fogColor = vec4(40.0/255.0, 45.0/255.0, 50.0/255.0, 1.0);
//const vec4 fogColor = vec4(70.0/255.0, 85.0/255.0, 100.0/255.0, 1.0);
//const vec4 fogColor = vec4(37.0/255.0, 35.0/255.0, 33.0/255.0, 1.0);
//const vec4 fogColor = vec4(56.0/255.0, 95.0/255.0, 141.0/255.0, 1.0);
//const vec4 fogColor = vec4(120.0/255.0, 155.0/255.0, 170.0/255.0, 1.0);
//const vec4 fogColor = vec4(85.0/255.0, 43.0/255.0, 0.0, 1.0);
//const vec4 fogColor = vec4(58.0/255.0, 82.0/255.0, 101.0/255.0, 1.0);
//const vec4 fogColor = vec4(21.0/255.0, 23.0/255.0, 19.0/255.0, 1.0); //d2_coast_09
const vec4 fogColor = vec4(140.0/255.0, 157.0/255.0, 176.0/255.0, 1.0); //d2_coast_09

#ifdef BUMP
#define OO_SQRT_3 0.57735025882720947
/*
const vec3 bumpBasis[3] = {
	vec3( 0.81649661064147949, 0.0, OO_SQRT_3 ),
	vec3(  -0.40824833512306213, 0.70710676908493042, OO_SQRT_3 ),
	vec3(  -0.40824821591377258, -0.7071068286895752, OO_SQRT_3 )
};
*/
const vec3 bumpBasis0 = vec3(  0.81649661064147949, 0.0, OO_SQRT_3 );
const vec3 bumpBasis1 = vec3( -0.40824833512306213, 0.70710676908493042, OO_SQRT_3 );
const vec3 bumpBasis2 = vec3( -0.40824821591377258, -0.7071068286895752, OO_SQRT_3 );
#endif

void main()
{
	vec4 baseColor = texture2D(u_tex, v_uv);
#ifdef ALPHA_TEST
	if(baseColor.a < 0.4)
		discard;
#endif
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);
	vec3 normal = v_normal;
	float specularFactor = 1.0;
	//todo: $basealphaenvmapmask
	specularFactor = 1.0-baseColor.a;
#ifdef BUMP
	vec4 bump = texture2D(u_bumpmap, v_uv,-1.0);
	//bump *= bump;gl_FragColor = vec4(bump.r+bump.g+bump.b); return;
	specularFactor = bump.a;
#if (BUMP==1)
	bump = bump*2.0-1.0;
#endif
#if (BUMP==2)
	bump *= bump;
	diffuse = texture2D(u_lmtex, v_lmuv12.xy).rgb*bump.x+
			texture2D(u_lmtex, v_lmuv12.zw).rgb*bump.y+
			texture2D(u_lmtex, v_lmuv3).rgb*bump.z;
	//bump.xyz = normalize(bumpBasis[0]*bump.x+bumpBasis[1]*bump.y+bumpBasis[2]*bump.z);
	bump.xyz = normalize(bumpBasis0*bump.x+bumpBasis1*bump.y+bumpBasis2*bump.z);
#else
	//vec3 dp = vec3(dot(bump,bumpBasis[0]),dot(bump,bumpBasis[1]),dot(bump,bumpBasis[2]));
	vec3 dp = vec3(dot(bump.xyz,bumpBasis0),dot(bump.xyz,bumpBasis1),dot(bump.xyz,bumpBasis2));
	dp*=dp;
	diffuse = texture2D(u_lmtex, v_lmuv12.xy).rgb*dp.x+
			texture2D(u_lmtex, v_lmuv12.zw).rgb*dp.y+
			texture2D(u_lmtex, v_lmuv3).rgb*dp.z;
#endif
	vec3 binormal = cross(normal,v_tangent);
	normal = normalize(normal*bump.z+v_tangent*bump.y+binormal*bump.x);
	//normal = v_tangent;
#else
	diffuse = texture2D(u_lmtex, v_lmuv12.xy).rgb;
#endif
#ifdef ENVMAP
	specularFactor = pow(specularFactor, 1.0/2.2);
	vec3 viewDir = normalize(u_cameraPos - v_position);
	//vec3 reflectLook = reflect(-viewDir, normal);
	vec3 reflectLook = (2.0*(dot( normal, viewDir ))*normal) - (dot( normal, normal )*viewDir);
	reflectLook.z*=-1.0;
	reflectLook = reflectLook.xzy;
	specular = 2.0*textureCube(u_envmap, reflectLook).rgb;
	specular *= specularFactor;
	specular *= u_envmapTint;
	//contrast
	specular = specular*specular;//contrast = 1
	//saturation
	vec3 grey = vec3(dot(specular,vec3(0.299, 0.587, 0.114)));
	specular = mix(grey,specular,0.2);

	//fresnel
	float fresnel = 1.0 - dot( normal, viewDir );
	fresnel = pow( fresnel, 5.0 );//5.0
	//fresnel = mix(fresnel,1.0,0.1);
	//fresnel = fresnel * g_OneMinusFresnelReflection + g_FresnelReflection;
	fresnel = fresnel * (1.0-0.5) + 0.5;
	specular *= fresnel;

#endif

	//flashlight
#ifdef FLASHLIGHT
	vec3 toLight = u_lightPos-v_position;
	float dist = length(toLight);
	toLight /= dist;
	float t = 1.0;//dot(toLight, u_lightDir);
	if(t>0.9)
	{
		float atten = 1.0/(1.0+0.1*dist/*+0.02*(dist*dist)*/);
		float intencity = clamp((t-0.9)/0.06,0.0,1.0);
		diffuse += max(dot(normal, toLight),0.0)*intencity*atten*0.5;
	}
#endif
	//
	gl_FragColor = vec4(baseColor.rgb*diffuse*2.0*overbright+specular, baseColor.a);
#ifdef FOG
	gl_FragColor = mix(fogColor, gl_FragColor, clamp(1.0 /exp(abs(gl_FragCoord.z/gl_FragCoord.w) * 0.02), 0.0, 1.0));
#endif
	//gl_FragColor = gl_FragColor*0.6;
	//gl_FragColor = pow(vec4(pow(baseColor.rgb,2.2) * diffuse * 2.0 * overbright, baseColor.a),1.0/2.2);
	//gl_FragColor = pow(vec4(baseColor.rgb * diffuse * 2.0 * overbright, baseColor.a),1.0/2.2);
	//gl_FragColor = vec4(baseColor.rgb * pow(diffuse * 2.0 * overbright,1.0/2.2), baseColor.a);
	//gl_FragColor = baseColor * vec4(pow(diffuse*2.0,0.9),1.0);
	//gl_FragColor.rgb = diffuse*2.0;
	//gl_FragColor = baseColor;
	//gl_FragColor.rgb = normalize(v_normal.xzy)*0.5+0.5;
	//gl_FragColor = vec4(dot(toLight, u_lightDir));
#ifdef ENVMAP
	//gl_FragColor = mix(gl_FragColor,vec4(specular,1.0),0.99);
	//gl_FragColor = vec4(fresnel);
	//gl_FragColor = 2.0*textureCube(u_envmap, reflectLook);//,7.0);
	//gl_FragColor.rgb = u_envmapTint*specularFactor*fresnel;
	//gl_FragColor.rgb = textureCube(u_envmap, reflectLook).rgb*specularFactor;
	//gl_FragColor.rgb = 2.0*textureCube(u_envmap, reflectLook).rgb*u_envmapTint;
	//gl_FragColor.rgb = u_envmapTint;
#endif
	
#ifdef BUMP
	//gl_FragColor = bump*0.5+0.5;
	//gl_FragColor.rgb = normal;//*0.5+0.5;
	//gl_FragColor.rgb = v_tangent*0.5+0.5;
#endif
	//gl_FragColor *= 0.7;
}
