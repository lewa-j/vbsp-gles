
#include <gtc/type_ptr.hpp>

#include "engine.h"
#include "log.h"

#include "graphics/platform_gl.h"
#include "renderer/progs_manager.h"
#include "renderer/render_vars.h"

void glslProgsManager::Init(Camera *c)
{
	cam = c;
	mtxUpdated.reset();
	lastProgType = eProgType_NULL;
	lastProg = 0;

	colorProg.CreateFromFile("generic", "color");
	colorProg.u_mvpMtx = colorProg.GetUniformLoc("u_mvpMtx");
	u_color = colorProg.GetUniformLoc("u_color");

	basetextureProg.CreateFromFile("generic", "texture");
	basetextureProg.u_mvpMtx = basetextureProg.GetUniformLoc("u_mvpMtx");

	unlitAlphaTestProg.CreateFromFile("generic", "texture", "#define ALPHA_TEST 1\n");
	unlitAlphaTestProg.u_mvpMtx = unlitAlphaTestProg.GetUniformLoc("u_mvpMtx");

	lightmapProg.CreateFromFile("generic", "lightmap");
	lightmapProg.u_mvpMtx = lightmapProg.GetUniformLoc("u_mvpMtx");
	int lmtex = lightmapProg.GetUniformLoc("u_lmtex");
	lightmapProg.Use();
	glUniform1i(lmtex, 1);

	lightmapAlphatestProg.CreateFromFile("generic", "lightmap", "#define ALPHA_TEST 1\n");
	lightmapAlphatestProg.u_mvpMtx = lightmapAlphatestProg.GetUniformLoc("u_mvpMtx");
	lmtex = lightmapAlphatestProg.GetUniformLoc("u_lmtex");
	lightmapAlphatestProg.Use();
	glUniform1i(lmtex, 1);

	flashlightProg.CreateFromFile("generic", "lightmap", "#define FLASHLIGHT 1\n");
	flashlightProg.u_mvpMtx = flashlightProg.GetUniformLoc("u_mvpMtx");
	flashlightProg.u_modelMtx = flashlightProg.GetUniformLoc("u_modelMtx");
	u_lightPos = flashlightProg.GetUniformLoc("u_lightPos");
	u_lightDir = flashlightProg.GetUniformLoc("u_lightDir");
	lmtex = flashlightProg.GetUniformLoc("u_lmtex");
	flashlightProg.Use();
	glUniform1i(lmtex, 1);

	lightmapEnvProg.CreateFromFile("generic", "lightmap", "#define ENVMAP 1\n");
	lightmapEnvProg.u_mvpMtx = lightmapEnvProg.GetUniformLoc("u_mvpMtx");
	lightmapEnvProg.u_modelMtx = lightmapEnvProg.GetUniformLoc("u_modelMtx");
	u_cameraPos_Env = lightmapEnvProg.GetUniformLoc("u_cameraPos");
	u_envmapTint_Env = lightmapEnvProg.GetUniformLoc("u_envmapTint");
	lmtex = lightmapEnvProg.GetUniformLoc("u_lmtex");
	int u_envmap = lightmapEnvProg.GetUniformLoc("u_envmap");
	lightmapEnvProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_envmap, 2);

	lightmapBumpProg.CreateFromFile("generic", "lightmap", "#define BUMP 1\n");
	lightmapBumpProg.u_mvpMtx = lightmapBumpProg.GetUniformLoc("u_mvpMtx");
	lightmapBumpProg.u_modelMtx = lightmapBumpProg.GetUniformLoc("u_modelMtx");
	lmtex = lightmapBumpProg.GetUniformLoc("u_lmtex");
	int u_bumpmap = lightmapBumpProg.GetUniformLoc("u_bumpmap");
	lightmapBumpProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_bumpmap, 3);

	lightmapSSBumpProg.CreateFromFile("generic", "lightmap", "#define BUMP 2\n");
	lightmapSSBumpProg.u_mvpMtx = lightmapSSBumpProg.GetUniformLoc("u_mvpMtx");
	lightmapSSBumpProg.u_modelMtx = lightmapSSBumpProg.GetUniformLoc("u_modelMtx");
	lmtex = lightmapSSBumpProg.GetUniformLoc("u_lmtex");
	u_bumpmap = lightmapSSBumpProg.GetUniformLoc("u_bumpmap");
	lightmapSSBumpProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_bumpmap, 3);

	lightmapBumpEnvProg.CreateFromFile("generic", "lightmap", "#define ENVMAP 1\n#define BUMP 1\n");
	lightmapBumpEnvProg.u_mvpMtx = lightmapBumpEnvProg.GetUniformLoc("u_mvpMtx");
	lightmapBumpEnvProg.u_modelMtx = lightmapBumpEnvProg.GetUniformLoc("u_modelMtx");
	u_cameraPos_BumpEnv = lightmapBumpEnvProg.GetUniformLoc("u_cameraPos");
	u_envmapTint_BumpEnv = lightmapBumpEnvProg.GetUniformLoc("u_envmapTint");
	lmtex = lightmapBumpEnvProg.GetUniformLoc("u_lmtex");
	u_envmap = lightmapBumpEnvProg.GetUniformLoc("u_envmap");
	u_bumpmap = lightmapBumpEnvProg.GetUniformLoc("u_bumpmap");
	//Log("lmbe a_tangent %d\n",lightmapBumpEnvProg.GetAttribLoc("a_tangent"));
	lightmapBumpEnvProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_envmap, 2);
	glUniform1i(u_bumpmap, 3);

	lightmapSSBumpEnvProg.CreateFromFile("generic", "lightmap", "#define ENVMAP 1\n#define BUMP 2\n");
	lightmapSSBumpEnvProg.u_mvpMtx = lightmapSSBumpEnvProg.GetUniformLoc("u_mvpMtx");
	lightmapSSBumpEnvProg.u_modelMtx = lightmapSSBumpEnvProg.GetUniformLoc("u_modelMtx");
	u_cameraPos_SSBumpEnv = lightmapSSBumpEnvProg.GetUniformLoc("u_cameraPos");
	u_envmapTint_SSBumpEnv = lightmapSSBumpEnvProg.GetUniformLoc("u_envmapTint");
	lmtex = lightmapSSBumpEnvProg.GetUniformLoc("u_lmtex");
	u_envmap = lightmapSSBumpEnvProg.GetUniformLoc("u_envmap");
	u_bumpmap = lightmapSSBumpEnvProg.GetUniformLoc("u_bumpmap");
	//Log("lmssbe a_tangent %d\n",lightmapSSBumpEnvProg.GetAttribLoc("a_tangent"));
	lightmapSSBumpEnvProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_envmap, 2);
	glUniform1i(u_bumpmap, 3);

	worldvertextransitionProg.CreateFromFile("worldvertextransition", "worldvertextransition");
	worldvertextransitionProg.u_mvpMtx = worldvertextransitionProg.GetUniformLoc("u_mvpMtx");
	lmtex = worldvertextransitionProg.GetUniformLoc("u_lmtex");
	int u_tex2 = worldvertextransitionProg.GetUniformLoc("u_tex2");
	worldvertextransitionProg.Use();
	glUniform1i(lmtex, 1);
	glUniform1i(u_tex2, 2);

	//TODO wvt: flashlight

	modelUnlitProg.CreateFromFile("model","model");
	modelUnlitProg.u_mvpMtx = modelUnlitProg.GetUniformLoc("u_mvpMtx");
	modelUnlitProg.u_modelMtx = modelUnlitProg.GetUniformLoc("u_modelMtx");

	modelProg.CreateFromFile("model","model", "#define AMBIENT_LIGHT 1\n");
	modelProg.u_mvpMtx = modelProg.GetUniformLoc("u_mvpMtx");
	modelProg.u_modelMtx = modelProg.GetUniformLoc("u_modelMtx");
	u_ambientCube = modelProg.GetUniformLoc("u_ambientCube");
	//Log("modelProg u_ambientCube %d\n", u_ambientCube);


	modelAlphaTestProg.CreateFromFile("model","model", "#define ALPHA_TEST 1\n#define AMBIENT_LIGHT 1\n");
	modelAlphaTestProg.u_mvpMtx = modelAlphaTestProg.GetUniformLoc("u_mvpMtx");
	modelAlphaTestProg.u_modelMtx = modelAlphaTestProg.GetUniformLoc("u_modelMtx");

	refractProg.CreateFromFile("refract", "refract");
	refractProg.u_mvpMtx = refractProg.GetUniformLoc("u_mvpMtx");
	u_tex2 = refractProg.GetUniformLoc("u_dudvtex");
	refractProg.Use();
	glUniform1i(u_tex2, 2);
}

void glslProgsManager::StartFrame()
{
	mtxUpdated.reset();
	lastProg = 0;
	lastProgType = eProgType_NULL;
}

void glslProgsManager::SelectProg(eProgType prog, int flags)
{
	glslProg *pr = GetProg(prog,flags);
	//TODO check and update mtx here
	pr->Use();
	lastProg = pr;
	lastProgType = prog;

	//if(log_render)
	//	LOG("glslProgsManager::SelectProg progType %d, flags %d, prog %p\n", (int)prog, flags, pr);
}

glslProg *glslProgsManager::GetProg(eProgType prog, int flags)
{
	glslProg *pr;
	switch(prog)
	{
	case eProgType_NULL:
		EngineError("GetProg NULL");
		return 0;
	case eProgType_LightmappedGeneric:
		if(flags&LIGHTMAP_BIT)
		{
			if(flags&FLASHLIGHT_BIT)
				pr = &flashlightProg;
			else
			{
				if(flags&ENVMAP_BIT)
				{
					if(flags&SSBUMP_BIT)
						pr = &lightmapSSBumpEnvProg;
					else if(flags&BUMP_BIT)
						pr = &lightmapBumpEnvProg;
					else
						pr = &lightmapEnvProg;
				}
				else
				{
					if(flags&SSBUMP_BIT)
						pr = &lightmapSSBumpProg;
					else if(flags&BUMP_BIT)
						pr = &lightmapBumpProg;
					else
					{
						if(flags&ALPHA_TEST_BIT)
							pr = &lightmapAlphatestProg;
						else
							pr = &lightmapProg;
					}
				}
			}
		}
		else
		{
			pr = &basetextureProg;
		}
		break;
	case eProgType_WorldVertexTransition:
		pr = &worldvertextransitionProg;
		//TODO wvt: flashlight, unlit, bump
		break;
	case eProgType_Black:
	case eProgType_Color:
		pr = &colorProg;
		break;
	case eProgType_Unlit:
		if(flags&ALPHA_TEST_BIT)
			pr = &unlitAlphaTestProg;
		else
			pr = &basetextureProg;
		break;
	case eProgType_VertexlitGeneric:
		if(flags&AMBIENT_LIGHT_BIT){
			if(flags&ALPHA_TEST_BIT)
				pr = &modelAlphaTestProg;
			else
				pr = &modelProg;
		}else
			pr = &modelUnlitProg;
		break;
	case eProgType_Refract:
		pr = &refractProg;
		break;
	default:
		pr = &basetextureProg;
	}

	//if(log_render)
	//	LOG("GetProg progType %d, flags %d, prog %p\n", (int)prog, flags, pr);
	return pr;
}

void glslProgsManager::SetProg(glslProg *pr)
{
	pr->Use();
	lastProg = pr;

	if(log_render)
		LOG("SetProg prog %p\n", pr);
}

void glslProgsManager::SetMtx(glm::mat4 aModelMtx)
{
	//TODO update only current prog

	mvpMtx = cam->projMtx * cam->viewMtx * aModelMtx;
	float *aMVPMtx = glm::value_ptr(mvpMtx);

	glUniformMatrix4fv(lastProg->u_mvpMtx, 1, false, aMVPMtx);

	if(flashlight && lastProg==&flashlightProg)
	{
		//flashlightProg.Use();
		glUniformMatrix4fv(lastProg->u_modelMtx, 1, false, glm::value_ptr(aModelMtx));
		glm::vec3 lp = cam->pos+glm::vec3(0.1,-0.1,0);
		glUniform3fv(u_lightPos,1, &lp.x);
		glm::vec3 lightDir = glm::normalize(glm::mat3(glm::inverse(cam->viewMtx))*glm::vec3(0,0,1));
		glUniform3fv(u_lightDir,1, &lightDir.x);
	}

	if(lastProg == &lightmapEnvProg)
	{
		glUniformMatrix4fv(lastProg->u_modelMtx, 1, false, glm::value_ptr(aModelMtx));
		glUniform3fv(u_cameraPos_Env,1, &cam->pos.x);
	}

	if(lastProg == &lightmapBumpEnvProg)
	{
		glUniformMatrix4fv(lastProg->u_modelMtx, 1, false, glm::value_ptr(aModelMtx));
		glUniform3fv(u_cameraPos_BumpEnv,1, &cam->pos.x);
	}

	if(lastProg == &lightmapSSBumpEnvProg)
	{
		glUniformMatrix4fv(lastProg->u_modelMtx, 1, false, glm::value_ptr(aModelMtx));
		glUniform3fv(u_cameraPos_SSBumpEnv,1, &cam->pos.x);
	}
	if(lastProg == &modelProg || lastProg == &modelAlphaTestProg)
	{
		glUniformMatrix4fv(lastProg->u_modelMtx, 1, false, glm::value_ptr(aModelMtx));
	}
}

void glslProgsManager::SetColor(float r, float g, float b, float a)
{
	glUniform4f(u_color,r,g,b,a);
}

void glslProgsManager::SetColor(glm::vec4 col)
{
	glUniform4fv(u_color,1,glm::value_ptr(col));
}

void glslProgsManager::SetEnvmapTint(glm::vec3 tint)
{
	if(lastProg == &lightmapEnvProg)
		glUniform3fv(u_envmapTint_Env,1,glm::value_ptr(tint));
	else if(lastProg == &lightmapBumpEnvProg)
		glUniform3fv(u_envmapTint_BumpEnv,1,glm::value_ptr(tint));
	else if(lastProg == &lightmapSSBumpEnvProg)
		glUniform3fv(u_envmapTint_SSBumpEnv,1,glm::value_ptr(tint));
	else
		return;//EngineError("SetEnvmapTint not envProg");
}

void glslProgsManager::SetLeafAmbient(glm::vec3 *amb)
{
	if(lastProg == &modelProg || lastProg == &modelAlphaTestProg)
		glUniform3fv(u_ambientCube,6,(float*)amb);
}
