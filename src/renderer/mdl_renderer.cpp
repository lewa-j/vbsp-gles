
#include <glm.hpp>

#include "engine.h"
#include "log.h"
#include "graphics/platform_gl.h"
#include "graphics/glsl_prog.h"
#include "graphics/texture.h"

#include "renderer/render_vars.h"
#include "renderer/vbsp_material.h"
#include "file_format/mdl.h"


#define MDL_VBO 1

void model_t::Draw()
{
	if(!loaded)
		return;
	glCullFace(GL_FRONT);

	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
#if MDL_VBO
	vbo.Bind();
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)16);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)28);//normals
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)40);//uv
	ibo.Bind();
#else
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),((byte*)mdlheader.pVertexBase)+16);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),((byte*)mdlheader.pVertexBase)+28);//normals
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),((byte*)mdlheader.pVertexBase)+40);//uv
#endif
	//mstudiomodel_t* pModel = (mstudiomodel_t*)((mstudiobodyparts_t*)mdlheader.bodypartindex)[0].modelindex;
	//int numMeshes = pModel[0].nummeshes;
	//mstudiomesh_t* pMesh = (mstudiomesh_t*)pModel[0].meshindex;
	//modelProg.Use();
	for(size_t m = 0; m < numMeshes; m++)
	{
/*		int numVerts = pMesh[m].numvertices;
		int vertOffset = pMesh[m].vertexoffset;
		colorProg.Use();
		//glUniform4f(u_color,m*37%256/255.0f,(m*63+30)%256/255.0f,(m*9+100)%256/255.0f,1);
		glUniform4fv(u_color, 1, colors+m*3);
		glDrawArrays(GL_POINTS,vertOffset,numVerts);
*/
		Material *mat = materials[m];
		if(!mat)
			EngineError("Model mesh material is NULL");
		if(mat->transparent)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(0);
		}
		
		mat->Bind();

		if(log_render)
			LOG("glDrawElements %d %d model_t::Draw\n", indsOfs[m].Count, indsOfs[m].Offset*sizeof(int));
		glDrawElements(GL_TRIANGLES, indsOfs[m].Count, GL_UNSIGNED_INT, (void*)(indsOfs[m].Offset*sizeof(int)));
		if(mat->transparent)
		{
			glDisable(GL_BLEND);
			glDepthMask(1);
		}
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glCullFace(GL_BACK);
}

void model_t::Bind()
{
	if(!loaded)
		return;
	
	//glCullFace(GL_FRONT);
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	vbo.Bind();
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)16);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)28);//normals
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(mstudiovertex_t),(byte*)40);//uv
	ibo.Bind();

	//Material *mat = materials[0];
	//mat->Bind();
}

bool model_t::DrawInst(eRenderPass pass)
{
	if(!loaded)
		return false;
	bool skip=false;
	
	for(size_t m = 0; m < numMeshes; m++)
	//int m=0;
	{
		Material *mat = materials[m];
		if(pass==eRenderPass_Opaque)
		{
			if(mat->transparent)
			{
				skip = true;
				continue;
			}
			if(mat->program==eProgType_Refract)
			{
				needRefract = true;
				continue;
			}
			if(materials[m]->nocull)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				glEnable(GL_CULL_FACE);
			}
		}
		else if(pass==eRenderPass_Transparent)
		{
			if(mat->program==eProgType_Refract)
			{
				needRefract = true;
				skip = true;
				continue;
			}
			if(!mat->transparent)
			{
				continue;
			}
		}
		else if(pass==eRenderPass_Refract)
		{
			if(mat->program!=eProgType_Refract)
			{
				continue;
			}
		}
		mat->Bind();

		if(log_render)
			LOG("glDrawElements %d %d model_t::DrawInst\n", indsOfs[m].Count, indsOfs[m].Offset*sizeof(int));
		glDrawElements(GL_TRIANGLES, indsOfs[m].Count, GL_UNSIGNED_INT, (void*)(indsOfs[m].Offset*sizeof(int)));
	}
	return skip;
}

void model_t::Unbind()
{
	glCullFace(GL_BACK);
	
	vbo.Unbind();
	ibo.Unbind();
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}
