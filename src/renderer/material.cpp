
#include "engine.h"
#include "log.h"
#include "renderer/vbsp_material.h"

void Material::Bind()
{
	if(log_render)
		LOG("Material::Bind %p prog %d, tex %d\n", this, (int)program, texture->id);

	if(program==eProgType_Black)
	{
		g_progsManager.SetColor(0,0,0,1);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0);
		texture->Bind();
	}
}

void LightmappedGenericMaterial::Bind()
{
	if(log_render)
		LOG("LightmappedGenericMaterial::Bind %p prog %d, tex %d\n", this, (int)program, texture->id);

	//TODO bind lightmap here or in map_t::Draw() ?
	glActiveTexture(GL_TEXTURE0);
	texture->Bind();
}

void LightmappedEnvMaterial::Bind()
{
	if(log_render)
		LOG("LightmappedEnvMaterial::Bind %p prog %d, tex %d, env %d\n", this, (int)program, texture->id, envmap->id);

	//TODO bind lightmap here or in map_t::Draw() ?
	glActiveTexture(GL_TEXTURE2);
	envmap->Bind();
	glActiveTexture(GL_TEXTURE0);
	texture->Bind();
	g_progsManager.SetEnvmapTint(envmapTint);
}

void LightmappedBumpMaterial::Bind()
{
	if(log_render)
		LOG("LightmappedBumpMaterial::Bind %p prog %d, tex %d, bump %d\n", this, (int)program, texture->id, bumpmap->id);

	//TODO bind lightmap here or in map_t::Draw() ?
	glActiveTexture(GL_TEXTURE3);
	bumpmap->Bind();
	glActiveTexture(GL_TEXTURE0);
	texture->Bind();
}

void LightmappedBumpEnvMaterial::Bind()
{
	if(log_render)
		LOG("LightmappedBumpEnvMaterial::Bind %p prog %d, tex %d, bump %d, env %d\n", this, (int)program, texture->id, bumpmap->id, envmap->id);

	//TODO bind lightmap here or in map_t::Draw() ?
	glActiveTexture(GL_TEXTURE3);
	bumpmap->Bind();
	glActiveTexture(GL_TEXTURE2);
	envmap->Bind();
	glActiveTexture(GL_TEXTURE0);
	texture->Bind();
	g_progsManager.SetEnvmapTint(envmapTint);
}

void WorldVertexTransitionMaterial::Bind()
{
	//EngineError("WorldVertexTransitionMaterial::Bind");
	if(log_render)
		LOG("wvt material bind %d %d\n",texture->id, texture2->id);

	glActiveTexture(GL_TEXTURE2);
	texture2->Bind();
	glActiveTexture(GL_TEXTURE0);
	texture->Bind();
}

void RefractMaterial::Bind()
{
	EngineError("RefractMaterial::Bind");
}

