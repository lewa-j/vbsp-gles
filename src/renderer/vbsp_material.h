
#pragma once

#include <string>

#include "graphics/platform_gl.h"

#define TEXTURE_NOMIP_BIT 1

#include "graphics/texture.h"
#include "renderer/progs_manager.h"
#include "renderer/render_vars.h"
extern Texture defaultTex;

class Material
{
public:
	//glslProg *program;
	eProgType program;
	Texture *texture;
	int flags;
	bool transparent;
	bool additive;
	bool nocull;
	
	Material():program(eProgType_NULL), texture(0), flags(0), transparent(0), additive(0), nocull(0)
	{}
	//Material(glslProg *p, Texture *t):program(p),texture(t)
	Material(eProgType p, Texture *t):program(p), texture(t), flags(0), transparent(0), additive(0), nocull(0)
	{}
	Material(eProgType p, Texture *t, bool tr, bool add, bool at, bool nc):program(p),texture(t),transparent(tr),additive(add),nocull(nc)
	{
		flags = 0;
		if(at)
			flags |= ALPHA_TEST_BIT;
	}
	
	virtual void Bind();
};

class LightmappedGenericMaterial: public Material
{
public:
	LightmappedGenericMaterial(eProgType p, Texture *t, bool tr, bool at, bool nc):Material(p,t,tr,0,at,nc)
	{}
	void Bind();
};

class LightmappedEnvMaterial: public Material
{
	Texture *envmap;
	glm::vec3 envmapTint;
public:
	LightmappedEnvMaterial(eProgType p, Texture *t, Texture *env, glm::vec3 envtint, bool tr):Material(p,t,tr,0,0,0), envmap(env),envmapTint(envtint)
	{
		flags = ENVMAP_BIT;
	}
	void Bind();
};

class LightmappedBumpMaterial: public Material
{
	Texture *bumpmap;
public:
	LightmappedBumpMaterial(eProgType p, Texture *t, Texture *bump, bool tr, bool ssbump):Material(p,t,tr,0,0,0), bumpmap(bump)
	{
		flags = BUMP_BIT;
		if(ssbump)
			flags |= SSBUMP_BIT;
	}
	void Bind();
};

class LightmappedBumpEnvMaterial: public Material
{
	Texture *bumpmap;
	Texture *envmap;
	glm::vec3 envmapTint;
public:
	LightmappedBumpEnvMaterial(eProgType p, Texture *t, Texture *bump, Texture *env, glm::vec3 envtint, bool tr, bool ssbump):Material(p,t,tr,0,0,0), bumpmap(bump), envmap(env), envmapTint(envtint)
	{
		flags = BUMP_BIT|ENVMAP_BIT;
		if(ssbump)
			flags |= SSBUMP_BIT;
	}
	void Bind();
};

class WorldVertexTransitionMaterial: public Material
{
public:
	Texture *texture2;
	
	WorldVertexTransitionMaterial():Material(),texture2(0)
	{}
	WorldVertexTransitionMaterial(eProgType p, Texture *t, Texture *t2):Material(p, t)
	{
		texture2 = t2;
	}
	
	void Bind();
};

class RefractMaterial: public Material
{
public:
	Texture *normalmap;

	RefractMaterial(eProgType p, Texture *t, Texture *t2): Material(p, t)
	{
		normalmap = t2;
		transparent = true;
	}

	void Bind();
};

#include <string>
#include <map>
class MaterialManager
{
public:
	std::map<std::string, Material*> loadedMaterials;
	std::map<std::string, Texture*> loadedTextures;
	
	Material *GetMaterial(const char *name, bool model=false);
	Texture *GetTexture(const char *name, int loadFlags=0);
	void AddMaterial(const char *name, Material *mat);
private:
	Material *LoadMaterial(const char *name);
};

extern MaterialManager g_MaterialManager;
