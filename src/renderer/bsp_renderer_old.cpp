
#include <cstdio>

#include <vec2.hpp>
#include <vec3.hpp>
#include <mat3x4.hpp>
#include <mat4x4.hpp>
#include <gtc/type_ptr.hpp>

#include "log.h"
#include "renderer/platform_gl.h"
#include "renderer/gl_utils.h"
#include "renderer/glslProg.h"
#include "renderer/texture.h"
#include "renderer/camera.h"

#include "renderer/vbsp_mesh.h"
#include "renderer/material.h"
#include "file_format/mdl.h"
#include "world.h"
#include "renderer/progs_manager.h"

extern Camera camera;

extern Texture g_lightmaps[8];
extern Texture whiteTex;
extern Texture frameTex;

bool no_vis = 0;
bool draw_verts = 0;
bool draw_lightmaps = 1;
bool draw_entities = 0;
bool draw_static_props = 1;
bool draw_refract = 0;
bool draw_transparent_props = 1;
bool draw_prop_bboxes = 0;
bool draw_area_portals = 1;
bool needRefract = 0;

GLint lastLightmap = 0;
Material *lastMat = 0;

#if NO_BATCHING
void DrawMapFace(map_t *m, submesh_t submesh, int i);
#endif
/*
void Material::Bind()
{
	//if(log_render)
	//	LOG("material bind %d\n",texture->id);

	glActiveTexture(GL_TEXTURE0);
	if(texture)
		texture->Bind();
	//else
	//	defaultTex.Bind();
}

void WorldVertexTransitionMaterial::Bind()
{
	//if(log_render)
	//	LOG("wvt material bind %d %d\n",texture->id, texture2->id);

	glActiveTexture(GL_TEXTURE2);
	//if(texture2)
		texture2->Bind();
	//else
	//	defaultTex.Bind();
	glActiveTexture(GL_TEXTURE0);
	//if(texture)
		texture->Bind();
	//else
	//	defaultTex.Bind();
}

void RefractMaterial::Bind()
{
	//g_progsManager.SelectProg(program, 0);
	glActiveTexture(GL_TEXTURE2);
	//if(normalmap)
		normalmap->Bind();
	//else
	//	defaultTex.Bind();
	glActiveTexture(GL_TEXTURE0);
	frameTex.Bind();
	//if(texture)
	//	texture->Bind();
	//else
	//	defaultTex.Bind();
}
*/
void materialBatch_t::Draw()
{
	if(log_render)
		LOG("glDrawElements %d %d materialBatch_t::Draw\n", submesh.Count, submesh.Offset);
	//glDrawElements(GL_TRIANGLES, numInds, GL_UNSIGNED_INT, model->inds+opaqueBatches[i].indOffset);
	glDrawElements(GL_TRIANGLES, submesh.Count, GL_UNSIGNED_INT, (void *)(submesh.Offset*sizeof(int)));
}

void map_t::DrawEntities()
{
	glDisable(GL_DEPTH_TEST);
	g_progsManager.SelectProg(eProgType_Color,0);
	g_progsManager.SetColor(1.0f,1.0f,0.5f,1.0f);
	
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,entPos);
	if(log_render)
		LOG("glDrawArrays %d DrawEntities\n", numEnts);
	glDrawArrays(GL_POINTS,0,numEnts);
	glEnable(GL_DEPTH_TEST);
	CheckGLError("DrawEntities", __FILE__, __LINE__);
}

#include <bitset>
void DrawBoxLines(glm::mat4 mtx, BoundingBox bbox, glm::vec4 color);
void UpdateRefract();

bool skipped = false;
void map_t::DrawStaticPropsOpaque()
{
	if(!numStaticPropsBatches)
		return;
	//calc vis
	if(numLeafs == 0)
		no_cull = 1;
	if(!no_cull&&!lock_frustum)
	{
		if(log_render)
			LOG("start calc vis\n");
		for(int i=0; i<numStaticPropsBatches; i++)
		{
			if(!staticPropsBatches[i].model)
			{
				//not visible
			}
			else
			{
				/*if(leafs[cameraLeaf].cluster == -1)
				{
					staticPropsBatches[i].vis.set();
					continue;
				}*/
				staticPropsBatches[i].vis.reset();
				for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
				{
					bool pvis = false;
					if(no_vis||	leafs[cameraLeaf].cluster == -1)
						pvis = true;
					else
					{
						for(int l=0; l<staticPropsBatches[i].insts[j].leafs.count; l++)
						{
							if(TestVisLeaf(staticPropsBatches[i].insts[j].leafs.leafs[l], cameraLeaf))
							{
								pvis = true;
								break;
							}
						}
					}
					if(pvis)
					{
						glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
						/*
						glm::vec4 sphere = modelMtx*glm::vec4(0,0,0,1.0f);
						sphere.w = 1.0;
						if(no_cull||camera.frustum.Contains(sphere))
						*/
						BoundingBox bbox = staticPropsBatches[i].model->bbox;
						//bbox.min = glm::vec3(modelMtx*glm::vec4(bbox.min,1.0));
						//bbox.max = glm::vec3(modelMtx*glm::vec4(bbox.max,1.0));
						if(glm::distance(camera.pos,glm::vec3(modelMtx*glm::vec4(0,0,0,1.0f)))<staticPropsBatches[i].dist)
						if(camera.frustum.Contains(bbox,modelMtx))
						{
							staticPropsBatches[i].vis.set(j);
						}
					}
				}
			}
		}
		if(log_render)
			LOG("finish calc vis\n");
	}
	//draw
	//glDisable(GL_DEPTH_TEST);
	skipped = false;
	if(log_render)
		LOG("start draw staticPropsBatches\n");
	for(int i=0; i<numStaticPropsBatches; i++)
	{
		if(!staticPropsBatches[i].model)
		{
			glDisable(GL_DEPTH_TEST);
			g_progsManager.SelectProg(eProgType_Color,0);
			g_progsManager.SetMtx(mapMtx);
			if(i==0)
				g_progsManager.SetColor(0.5f,1.0f,1.0f,1.0f);
			else
				g_progsManager.SetColor(0.5f,0.5f,1.0f,1.0f);
			glBindBuffer(GL_ARRAY_BUFFER,0);

			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,(float*)staticPropsBatches[i].positions);
			if(log_render)
				LOG("glDrawArrays %d DrawStaticPropsOpaque\n", staticPropsBatches[i].numInsts);
			glDrawArrays(GL_POINTS,0,staticPropsBatches[i].numInsts);
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			if(!staticPropsBatches[i].insts)
				continue;
			int progFlags = 0;
			//glslProg *pr = &modelProg;
			if(staticPropsBatches[i].model->materials[0]->alphatest)
				progFlags |= ALPHA_TEST_BIT;
			//	pr = &modelAlphatestProg;
			//pr->Use();
			if(staticPropsBatches[i].model->materials[0]->program==eProgType_Black)
			{
				g_progsManager.SelectProg(eProgType_Black, progFlags);
				g_progsManager.SetColor(0,0,0,1);
			}
			else
			{
				g_progsManager.SelectProg(eProgType_VertexlitGeneric, progFlags);
			}
			//glslProg *pr = g_progsManager.SelectProg(eProgType_VertexlitGeneric,progFlags);
			staticPropsBatches[i].model->Bind();

			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				if(no_cull||staticPropsBatches[i].vis.test(j))
				{
					glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
					//glm::mat4 mvpMtx = camera.projMtx*camera.viewMtx*modelMtx;
					//glUniformMatrix4fv(pr->u_mvpMtx, 1, false, glm::value_ptr(mvpMtx));
					//glUniformMatrix4fv(pr->u_modelMtx, 1, false, glm::value_ptr(modelMtx));
					g_progsManager.SetMtx(modelMtx);
					skipped = staticPropsBatches[i].model->DrawInst(eRenderPass_Opaque)||skipped;
				}
			}
		}
	}
	if(log_render)
		LOG("finish draw opaque static props\n");
	glEnable(GL_CULL_FACE);
	CheckGLError("DrawStaticProps opaque", __FILE__, __LINE__);

	staticPropsBatches[0].model->Unbind();

	if(draw_prop_bboxes)
	{
		g_progsManager.SelectProg(eProgType_Color,0);
		glDisable(GL_DEPTH_TEST);
		for(int i=0; i<numStaticPropsBatches; i++)
		{
			if(!staticPropsBatches[i].model)
				continue;
			if(!staticPropsBatches[i].insts)
				continue;
			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
				if(no_cull||staticPropsBatches[i].vis.test(j))
				{
					DrawBoxLines(modelMtx, staticPropsBatches[i].model->bbox, glm::vec4(1.0));
				}
				else
				{
					//DrawBoxLines(modelMtx, staticPropsBatches[i].model->bbox, glm::vec4(0.5));
				}
			}

		}
		glEnable(GL_DEPTH_TEST);
	}

	CheckGLError("DrawStaticPropsOpaque", __FILE__, __LINE__);
}

void map_t::DrawStaticPropsTransparent()
{
	if(!numStaticPropsBatches)
		return;
	//draw
	//glDisable(GL_DEPTH_TEST);

	needRefract = false;
	//transparent
	if(skipped&&draw_transparent_props)
	{
		//glslProg *pr =
		g_progsManager.SelectProg(eProgType_VertexlitGeneric,0);
		for(int i=0; i<numStaticPropsBatches; i++)
		{
			if(!staticPropsBatches[i].model)
				continue;
			if(!staticPropsBatches[i].insts)
				continue;
			if(!staticPropsBatches[i].model->transparent)
				continue;

			staticPropsBatches[i].model->Bind();
			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				if(no_cull||staticPropsBatches[i].vis.test(j))
				{
					glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
					//glm::mat4 mvpMtx = camera.projMtx*camera.viewMtx*modelMtx;
					//glUniformMatrix4fv(pr->u_mvpMtx, 1, false, glm::value_ptr(mvpMtx));
					//glUniformMatrix4fv(pr->u_modelMtx, 1, false, glm::value_ptr(modelMtx));
					g_progsManager.SetMtx(modelMtx);
					staticPropsBatches[i].model->DrawInst(eRenderPass_Transparent);
				}
			}
		}
	}
#if 1
	//refract
	if(needRefract&&draw_refract)
	{
		UpdateRefract();
		//glslProg *pr = 
		g_progsManager.SelectProg(eProgType_Refract,0);
		for(int i=0; i<numStaticPropsBatches; i++)
		{
			if(!staticPropsBatches[i].model)
				continue;
			if(!staticPropsBatches[i].insts)
				continue;
			//TODO Check refract before bind
			staticPropsBatches[i].model->Bind();
			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				if(no_cull||staticPropsBatches[i].vis.test(j))
				{
					glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
					//glm::mat4 mvpMtx = camera.projMtx*camera.viewMtx*modelMtx;
					//glUniformMatrix4fv(pr->u_mvpMtx, 1, false, glm::value_ptr(mvpMtx));
					//glUniformMatrix4fv(g_progsManager.refractProg.u_modelMtx, 1, false, glm::value_ptr(modelMtx));
					g_progsManager.SetMtx(modelMtx);
					staticPropsBatches[i].model->DrawInst(eRenderPass_Refract);
				}
			}
		}
		glActiveTexture(GL_TEXTURE0);
		defaultTex.Bind();
		CheckGLError("DrawStaticProps refract", __FILE__, __LINE__);
	}
#endif
	staticPropsBatches[0].model->Unbind();

	//glEnable(GL_DEPTH_TEST);
	CheckGLError("DrawStaticPropsTransparent", __FILE__, __LINE__);
}

void map_t::BindBSP()
{
	modelVbo.Bind();
	modelIbo.Bind();

	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts);//pos
	if(flashlight)
	{
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+3);//nrm
	}
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+6);//uv
	if(draw_lightmaps)
	{
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+8);//lmuv
	}
	CheckGLError("BindBSP", __FILE__, __LINE__);
}

void map_t::Draw()
{
	cameraLeaf = TraceTree(glm::inverse(glm::mat3(mapMtx))*camera.pos);

	//Draw world

	defaultTex.Bind();

	BindBSP();

	lastMat = 0;
	lastLightmap = -1;
	for(int i=0; i<numOpaqueBatches; i++)
	{
		if(opaqueBatches[i].submesh.Count == 0)
			continue;

		Material *mat = opaqueBatches[i].material;

		if(mat->transparent)
		{
			EngineError("Transparent material in opaque list!");
		}
		//if(mat.tools)
		//	continue;

		int progFlags=0;
		eProgType pt;
		if(flashlight)
			progFlags |= FLASHLIGHT_BIT;

		if(draw_lightmaps&&mat->program!=eProgType_Unlit)
		{
			progFlags |= LIGHTMAP_BIT;
			pt = eProgType_LightmappedGeneric;
			glActiveTexture(GL_TEXTURE1);
			int lmi = opaqueBatches[i].lightmap;
			if(lmi < 0)
				whiteTex.Bind();
			else
				if(lastLightmap != lmi)
				{
					g_lightmaps[lmi].Bind();
					lastLightmap = lmi;
					if(log_render)
						LOG("batch %d lmi %d\n", i, lmi);
				}
			glActiveTexture(GL_TEXTURE0);
		}
		else
		{
			pt = eProgType_Unlit;
			//mat->program;
		}

		g_progsManager.SelectProg(pt, progFlags);
		g_progsManager.SetMtx(mapMtx);
		if(log_render)
			LOG("select prog flags %d\n", progFlags);

		bool newTex = false;
		if(lastMat!=mat)
		{
			if(lastMat)
			{
				if(lastMat->texture != mat->texture)
					newTex = true;
			}
			else
				newTex = true;
		}
		if(newTex)
		{
			//if(log_render)
			//	LOG("batch %d mat %p tex %d\n", i, mat, mat->texture->id);
			lastMat = mat;
			mat->Bind();
		}
		CheckGLError("DrawMap bind textures", __FILE__, __LINE__);
#if NO_BATCHING
		for(GLuint j=0; j<materialFaces[i].size(); j++)
		{
			int f = materialFaces[i][j];
			DrawMapFace(this,model->submeshes[f],f);
		}
#else
		//if(log_render)
		//	LOG("glDrawElements %d %d map_t::Draw opaque\n", opaqueBatches[i].numInds, (int)model->inds+opaqueBatches[i].indOffset);
		//glDrawElements(GL_TRIANGLES, opaqueBatches[i].numInds, GL_UNSIGNED_INT, model->inds+opaqueBatches[i].indOffset);
		opaqueBatches[i].Draw();
		CheckGLError("DrawMap DrawElements", __FILE__, __LINE__);
#endif
	}
	if(log_render)
		LOG("finish opaque world\n");

	//glDisableVertexAttribArray(3);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//disp
	int progFlags=0;
	if(flashlight)
		progFlags |= FLASHLIGHT_BIT;
	if(draw_lightmaps)
		progFlags |= LIGHTMAP_BIT;
	g_progsManager.SelectProg(eProgType_WorldVertexTransition,progFlags);
	g_progsManager.SetMtx(mapMtx);
	for(int i = 0; i<numDispBatches; i++)
	{
		Material *mat = dispBatches[i].material;
		
		if(draw_lightmaps)
		{
			int lmi = dispBatches[i].lightmap;
			if(lmi < 0)
				whiteTex.Bind();
			else
				if(lastLightmap != lmi)
				{
					glActiveTexture(GL_TEXTURE1);
					g_lightmaps[lmi].Bind();
					lastLightmap = lmi;
					if(log_render)
						LOG("disp %d mat %p lmi %d\n", i, mat, lmi);
				}
		}
		else
		{
			glActiveTexture(GL_TEXTURE1);
			whiteTex.Bind();
		}
		if(lastMat != mat)
		{
			lastMat = mat;
			//if(mat)
				mat->Bind();
			//else
			//	glActiveTexture(GL_TEXTURE0);
		}
		if(mat->program!=eProgType_WorldVertexTransition)
		{
			glActiveTexture(GL_TEXTURE2);
			mat->texture->Bind();
		}
		glActiveTexture(GL_TEXTURE0);

		glEnableVertexAttribArray(1);

		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,dispBatches[i].verts);//pos
		glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,32,dispBatches[i].verts+3);//alpha
		glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,32,dispBatches[i].verts+4);//uv
		if(draw_lightmaps)
		{
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,32,dispBatches[i].verts+6);//lmuv
		}

		if(log_render)
			LOG("glDrawElements %d %d map_t::Draw disp\n", dispBatches[i].submesh.Count, dispBatches[i].submesh.Offset);
		glDrawElements(GL_TRIANGLES, dispBatches[i].submesh.Count, GL_UNSIGNED_INT, (void*)dispBatches[i].submesh.Offset );
	}
	if(log_render)
		LOG("finish displacements\n");

	if(draw_static_props)
	{
		glDisableVertexAttribArray(3);
		DrawStaticPropsOpaque();
		if(log_render)
			LOG("finish opaque props\n");
	}
	
	//transparent
#if 1
	BindBSP();

	glEnable(GL_BLEND);
	glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	lastMat = 0;
	lastLightmap = -1;
	for(int i=0; i<numTransparentBatches; i++)
	{
		if(transparentBatches[i].submesh.Count == 0)
			continue;

		Material *mat = transparentBatches[i].material;

		if(!mat->transparent)
		{
			EngineError("Opaque material in transparent list!");
		}
		/*
		if(mat->additive)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		*/
		int progFlags=0;
		eProgType pt;
		if(flashlight)
			progFlags |= FLASHLIGHT_BIT;
		if(draw_lightmaps&&mat->program!=eProgType_Unlit)
		{
			progFlags |= LIGHTMAP_BIT;
			pt = eProgType_LightmappedGeneric;
			glActiveTexture(GL_TEXTURE1);
			int lmi = transparentBatches[i].lightmap;
			if(lmi < 0)
				whiteTex.Bind();
			else
				if(lastLightmap != lmi)
				{
					g_lightmaps[lmi].Bind();
					lastLightmap = lmi;
					if(log_render)
						LOG("tr batch %d lmi %d\n", i, lmi);
				}
			glActiveTexture(GL_TEXTURE0);
		}
		else
		{
			pt = eProgType_Unlit;
			//mat->program;
		}
		g_progsManager.SelectProg(pt, progFlags);
		g_progsManager.SetMtx(mapMtx);
		if(log_render)
			LOG("select prog flags %d\n", progFlags);
		bool newTex = false;
		if(lastMat!=mat)
		{
			if(lastMat)
			{
				if(lastMat->texture != mat->texture)
					newTex = true;
			}
			else
				newTex = true;
		}
		if(newTex)
		{
			//if(log_render)
			//	LOG("tr batch %d mat %p tex %d\n", i, mat, mat->texture->id);
			lastMat = mat;
			mat->Bind();
		}
		CheckGLError("DrawMap transp bind textures", __FILE__, __LINE__);

#if NO_BATCHING
		for(GLuint j=0; j<materialFaces[i].size(); j++)
		{
			int f = materialFaces[i][j];
			DrawMapFace(this,model->submeshes[f],f);
		}
#else
		//if(log_render)
		//	LOG("glDrawElements %d %d map_t::Draw transparent\n", transparentBatches[i].numInds, (int)model->inds+transparentBatches[i].indOffset);
		//glDrawElements(GL_TRIANGLES, transparentBatches[i].numInds, GL_UNSIGNED_INT, model->inds+transparentBatches[i].indOffset);
		transparentBatches[i].Draw();
		CheckGLError("DrawMap transp DrawElements", __FILE__, __LINE__);
#endif
	}
	if(log_render)
		LOG("finish transparent world\n");

	if(draw_static_props)
	{
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		DrawStaticPropsTransparent();
		if(log_render)
			LOG("finish transparent props\n");
	}

	glDepthMask(1);
	glDisable(GL_BLEND);
#endif
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	
	//Draw all verts
	if(draw_verts)
	{
		g_progsManager.SelectProg(eProgType_Color,0);
		g_progsManager.SetMtx(mapMtx);
		g_progsManager.SetColor(1.0f,1.0f,1.0f,1.0f);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,mapmesh.verts);
		glDisable(GL_DEPTH_TEST);
		if(log_render)
			LOG("glDrawArrays %d map_t::Draw verts\n", mapmesh.numVerts);
		glDrawArrays(GL_POINTS,0,mapmesh.numVerts);
		glEnable(GL_DEPTH_TEST);
		CheckGLError("DrawMap draw points", __FILE__, __LINE__);
	}
	
	if(draw_entities)
	{
		DrawEntities();
	}

	if(draw_area_portals && numAreaPortals && areaPortals[0].m_nClipPortalVerts)
	{
		g_progsManager.SelectProg(eProgType_Color,0);
		g_progsManager.SetMtx(mapMtx);
		g_progsManager.SetColor(0.2f,0.2f,1.0f,1.0f);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,areaPortalVerts);
		glDisable(GL_DEPTH_TEST);
		for(int i = 0; i<numAreaPortals; i++)
		{
			if(log_render)
				LOG("glDrawArrays %d %d map_t::Draw area portals\n", areaPortals[i].m_FirstClipPortalVert, areaPortals[i].m_nClipPortalVerts);
			glDrawArrays(GL_LINE_LOOP,areaPortals[i].m_FirstClipPortalVert,areaPortals[i].m_nClipPortalVerts);
		}
		glEnable(GL_DEPTH_TEST);
		CheckGLError("DrawMap draw area portals", __FILE__, __LINE__);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if(log_render)
		LOG("finish draw map\n");
}

#if NO_BATCHING
void DrawMapFace(map_t *m, submesh_t submesh, int i)
{
	if(submesh.numVerts <= 0)
	{
		if(log_render) LOG("face %d without verts",i);
		return;
	}

#ifndef NO_LM
	if(lastLightmap!=m->lightmaps[i].id)
	{
		glActiveTexture(GL_TEXTURE1);
		m->lightmaps[i].Bind();
		glActiveTexture(GL_TEXTURE0);
		lastLightmap = m->lightmaps[i].id;
	}
#endif
	//if(!(i%4))
		glDrawArrays(GL_TRIANGLE_FAN,submesh.vertOffset,submesh.numVerts);
/*
	if(m->texInfos[m->faces[i].texinfo].texdata==36)
	{
		glDisable(GL_DEPTH_TEST);
		colorProg.Use();
		glDrawArrays(GL_LINE_STRIP,submesh.vertOffset,submesh.numVerts);
		lightmapProg.Use();
		glEnable(GL_DEPTH_TEST);
	}*/
}
#endif
