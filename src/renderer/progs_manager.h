
#pragma once

enum eProgType
{
	eProgType_NULL,
	eProgType_LightmappedGeneric,
	eProgType_WorldVertexTransition,
	eProgType_Color,
	eProgType_Unlit,
	eProgType_VertexlitGeneric,
	eProgType_Refract,
	eProgType_Black,
	eProgType_Water
//	eProgType_LastEnum
};

#define FLASHLIGHT_BIT 1
#define LIGHTMAP_BIT 2
#define ALPHA_TEST_BIT 4
#define ENVMAP_BIT 8
#define BUMP_BIT 16
#define SSBUMP_BIT 32
#define AMBIENT_LIGHT_BIT 64

#include <bitset>
#include <vec4.hpp>
#include <mat4x4.hpp>

#include "graphics/glsl_prog.h"
#include "renderer/camera.h"

class glslProgsManager
{
	glslProg colorProg;
	GLint u_color;
	glslProg basetextureProg;
	glslProg unlitAlphaTestProg;
	glslProg lightmapProg;
	glslProg lightmapAlphatestProg;
	glslProg flashlightProg;
	GLint u_lightPos;
	GLint u_lightDir;
	glslProg lightmapEnvProg;
	GLint u_cameraPos_Env;
	GLint u_envmapTint_Env;
	glslProg lightmapBumpEnvProg;
	GLint u_cameraPos_BumpEnv;
	GLuint u_envmapTint_BumpEnv;
	glslProg lightmapSSBumpEnvProg;
	GLint u_cameraPos_SSBumpEnv;
	GLint u_envmapTint_SSBumpEnv;
	glslProg lightmapBumpProg;
	glslProg lightmapSSBumpProg;
	glslProg worldvertextransitionProg;
	glslProg refractProg;
	glslProg modelUnlitProg;
	glslProg modelProg;
	GLint u_ambientCube;
	glslProg modelAlphaTestProg;

	Camera *cam;
	glm::mat4 mvpMtx;
	std::bitset<16> mtxUpdated;
	eProgType lastProgType;
	glslProg *lastProg;
public:
	void Init(Camera *c);
	void StartFrame();
	void SelectProg(eProgType prog, int flags);
	glslProg *GetProg(eProgType prog, int flags);
	void SetProg(glslProg *pr);
	void SetMtx(glm::mat4 aModelMtx);
	void SetColor(float r, float g, float b, float a);
	void SetColor(glm::vec4 col);
	void SetEnvmapTint(glm::vec3 tint);
	void SetLeafAmbient(glm::vec3 *amb);
};

extern glslProgsManager g_progsManager;
