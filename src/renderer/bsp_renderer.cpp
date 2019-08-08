
#include "log.h"
#include "graphics/gl_utils.h"
#include "graphics/platform_gl.h"
#include "renderer/progs_manager.h"
#include "world.h"
#include "renderer/render_list.h"

extern Texture g_lightmaps[8];
extern Texture whiteTex;
extern Camera camera;

extern mleaf_t *cameraLeaf;
extern mleaf_t *oldCameraLeaf;

bool draw_verts = 0;
bool draw_lightmaps = 1;
bool draw_entities = 0;
bool draw_static_props = 1;
bool no_vis = 0;
bool draw_prop_bboxes = 0;
bool draw_area_portals = 0;
bool r_bump = 0;
bool needRefract = 0;


int lastLightmap = -2;
Material *lastMat = 0;
int renderMats = 0;
int renderProps = 0;
RenderList g_renderList;
glm::vec3 curViewPos;


void DrawBoxLines(glm::mat4 mtx, glm::vec3 mins, glm::vec3 maxs, glm::vec4 color);

void Mesh::Draw()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, verts);
	if(log_render)
		Log("glDrawArrays %d Mesh(%p)::Draw\n", numVerts,this);
	glDrawArrays(GL_POINTS, 0, numVerts);
}

void materialBatch_t::Draw()
{
	if(log_render)
		Log("glDrawElements %d %d materialBatch_t(%p)::Draw\n", submesh.Count, submesh.Offset,this);
	//glDrawElements(GL_TRIANGLES, numInds, GL_UNSIGNED_INT, model->inds+opaqueBatches[i].indOffset);
	glDrawElements(GL_TRIANGLES, submesh.Count, GL_UNSIGNED_INT, (void *)(submesh.Offset*sizeof(int)));
}

void map_t::BindBSP()
{
	modelVbo.Bind();
	modelIbo.Bind();

	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts);//pos
	if(1||flashlight){
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+3);//nrm
	}
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+6);//uv
	if(draw_lightmaps){
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3,4,GL_FLOAT,GL_FALSE,sizeof(worldvert_t), model->verts+8);//lmuv
	}
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(worldvert_t),model->verts+12);//tangent
	CheckGLError("BindBSP", __FILE__, __LINE__);
}

void BindLM(int lm)
{
	if(lastLightmap != lm){
		glActiveTexture(GL_TEXTURE1);
		if(lm < 0)
			whiteTex.Bind();
		else
			g_lightmaps[lm].Bind();
		if(log_render)
			Log("BindLM %d\n", lm);
		lastLightmap = lm;
		glActiveTexture(GL_TEXTURE0);
	}
}

void map_t::Draw()
{
	renderLeafs = 0;
	renderMats = 0;
	renderProps = 0;
	curframe++;

	if(!lock_frustum)
		curViewPos = glm::inverse(glm::mat3(mapMtx))*camera.pos;
	cameraLeaf = TraceTree(curViewPos);

	MarkLeaves();

	RecursiveWorldNode(nodes);

	CheckGLError("map_t::Draw pre BuildIndices", __FILE__, __LINE__);
	BuildIndices();

	g_renderList.Clean();
	g_renderList.SetMtx(mapMtx);
	BindBSP();

	lastLightmap = -1;

	glEnable(GL_DEPTH_TEST);
	for(int i=0; i<numOpaqueBatches; i++){
		if(!opaqueBatches[i].submesh.Count)
			continue;

		int progFlags=0;
		if(flashlight)
			progFlags |= FLASHLIGHT_BIT;
		if(draw_lightmaps&&opaqueBatches[i].lightmap!=-1)
			progFlags |= LIGHTMAP_BIT;

		//BindLM(opaqueBatches[i].lightmap);
		//opaqueBatches[i].material->Bind(progFlags);
		//opaqueBatches[i].Draw();
		g_renderList.Add(opaqueBatches[i].material,opaqueBatches[i].submesh,progFlags);
		renderMats++;
	}
	BindLM(0);
	g_renderList.Flush();
	modelVbo.Unbind();
	modelIbo.Unbind();
	
	lastMat = 0;
	//disp
	int progFlags=0;
	if(flashlight)
		progFlags |= FLASHLIGHT_BIT;
	if(draw_lightmaps)
		progFlags |= LIGHTMAP_BIT;
	g_progsManager.SelectProg(eProgType_WorldVertexTransition,progFlags);
	g_progsManager.SetMtx(mapMtx);
	for(int i = 0; i<numDispBatches; i++){
		Material *mat = dispBatches[i].material;
		
		if(draw_lightmaps){
			BindLM(dispBatches[i].lightmap);
		}else{
			glActiveTexture(GL_TEXTURE1);
			whiteTex.Bind();
		}
		if(lastMat != mat){
			lastMat = mat;
			//if(mat)
				mat->Bind();
			//else
			//	glActiveTexture(GL_TEXTURE0);
		}
		if(mat->program!=eProgType_WorldVertexTransition){
			glActiveTexture(GL_TEXTURE2);
			mat->texture->Bind();
		}
		glActiveTexture(GL_TEXTURE0);

		glEnableVertexAttribArray(1);

		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(dispvert_t),dispBatches[i].verts);//pos
		glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(dispvert_t),dispBatches[i].verts+3);//alpha
		glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(dispvert_t),dispBatches[i].verts+4);//uv
		if(draw_lightmaps){
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(dispvert_t),dispBatches[i].verts+6);//lmuv
		}

		if(log_render)
			Log("glDrawElements %d %d map_t::Draw disp\n", dispBatches[i].submesh.Count, dispBatches[i].submesh.Offset);
		glDrawElements(GL_TRIANGLES, dispBatches[i].submesh.Count, GL_UNSIGNED_INT, (void*)dispBatches[i].submesh.Offset );
	}
	if(log_render)
		Log("finish displacements\n");

	if(draw_static_props){
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);
		DrawStaticPropsOpaque();
		if(log_render)
			Log("finish opaque props\n");
	}

	//transparrent world
	g_renderList.Clean();
	BindBSP();
	for(int i=0; i<numTransparentBatches; i++){
		if(!transparentBatches[i].submesh.Count)
			continue;

		int progFlags=0;
		if(flashlight)
			progFlags |= FLASHLIGHT_BIT;
		if(draw_lightmaps&&transparentBatches[i].lightmap!=-1)
			progFlags |= LIGHTMAP_BIT;

		//BindLM(opaqueBatches[i].lightmap);
		g_renderList.Add(transparentBatches[i].material,transparentBatches[i].submesh,progFlags);
		renderMats++;
	}
	BindLM(0);
	glEnable(GL_BLEND);
	//glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_renderList.SetMtx(mapMtx);
	g_renderList.Flush();
	glDisable(GL_BLEND);
	//glDepthMask(1);
	//

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);

	if(draw_verts){
		glEnableVertexAttribArray(0);
		g_progsManager.SelectProg(eProgType_Color, 0);
		g_progsManager.SetMtx(mapMtx);
		g_progsManager.SetColor(0.5f, 0.5f, 0.5f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		mapmesh.Draw();
		glEnable(GL_DEPTH_TEST);
		CheckGLError("map_t::Draw points", __FILE__, __LINE__);
	}

	if(draw_entities){
		//DrawEntities();
	}

	if(draw_area_portals && numAreaPortals)// && areaPortals[0].m_nClipPortalVerts
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

	//draw cubemaps
	if(0)
	{
		glDisable(GL_DEPTH_TEST);
		g_progsManager.SetColor(1.0f,1.0f,1.0f,1.0f);
		glVertexAttribPointer(0,3,GL_INT,GL_FALSE,sizeof(bspcubemapsample_t),cubemapSamples);
		glDrawArrays(GL_POINTS,0,numCubemaps);
		glEnable(GL_DEPTH_TEST);
		CheckGLError("DrawMap draw cubemaps", __FILE__, __LINE__);
	}

	//draw leaves
	/*for(int i = 0; i<numLeafs; i++)
	{
		if(leafs[i].cluster == -1)
			continue;
		if(camera.frustum.Contains(leafs[i].mins,leafs[i].maxs))
		{
			DrawBoxLines(glm::mat4(1.0),leafs[i].mins,leafs[i].maxs,glm::vec4(1.0));
		}
		else
		{
			DrawBoxLines(glm::mat4(1.0),leafs[i].mins,leafs[i].maxs,glm::vec4(0.5));
		}
	}*/
}

void map_t::DrawStaticPropsOpaque()
{
	if(!numStaticPropsBatches)
		return;

	//vis
	for(int i = 0; i < numStaticPropsBatches; i++)
	{
		if(!staticPropsBatches[i].model)
			continue;

		for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
		{
			bool ivis = false;
			if(no_vis || cameraLeaf->cluster==-1)
				ivis = true;
			if(!ivis)
			{
				for(int l=0; l<staticPropsBatches[i].insts[j].leafs.count; l++)
				{
					if(TestVisLeaf(staticPropsBatches[i].insts[j].leafs.leafs[l], cameraLeaf))
					{
						ivis = true;
						break;
					}
				}
			}
			if(ivis)
			{
				glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
				BoundingBox bbox = staticPropsBatches[i].model->bbox;
				//if(glm::distance(camera.pos,glm::vec3(modelMtx*glm::vec4(0,0,0,1.0f)))<staticPropsBatches[i].dist)
				if(camera.frustum.Contains(bbox,modelMtx))
				{
					staticPropsBatches[i].visFrame = curVisFrame;
					staticPropsBatches[i].insts[j].visFrame = curVisFrame;
				}
			}
		}
	}

	//draw
	bool need_points = false;
	for(int i = 0; i < numStaticPropsBatches; i++)
	{
		if(!staticPropsBatches[i].model)
		{
			need_points=true;
		}
		else
		{
			if(!staticPropsBatches[i].insts || staticPropsBatches[i].visFrame!=curVisFrame)
				continue;
#if 0
			eProgType pt = eProgType_VertexlitGeneric;
			pt = staticPropsBatches[i].model->materials[0]->program;
			g_progsManager.SelectProg(pt, 0);
			staticPropsBatches[i].model->Bind();
			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				//if(no_cull||staticPropsBatches[i].vis.test(j))
				if(staticPropsBatches[i].insts[j].visFrame==curVisFrame)
				{
					glm::mat4 modelMtx = staticPropsBatches[i].insts[j].modelMtx;
					g_progsManager.SetMtx(modelMtx);
					//TODO fix
					g_progsManager.SetLeafAmbient(leafs[staticPropsBatches[i].insts[j].leafs.leafs[0]].ambientLightTemp);
					staticPropsBatches[i].model->DrawInst(eRenderPass_Opaque);
					renderProps++;
				}
			}
#endif
			for(int j = 0; j < staticPropsBatches[i].numInsts; j++)
			{
				if(staticPropsBatches[i].insts[j].visFrame==curVisFrame)
				{
					int prFlags=0;
					if(draw_lightmaps&&(leafLumpVersion==0||numAmbientInds!=0)){
						prFlags|=AMBIENT_LIGHT_BIT;
					}
					g_renderList.Add(staticPropsBatches[i].model,&staticPropsBatches[i].insts[j].modelMtx, staticPropsBatches[i].insts[j].lightLeaf->ambientLightTemp,prFlags);
					renderProps++;
				}
			}
		}
	}
	glCullFace(GL_FRONT);
	g_renderList.FlushModels();
	//staticPropsBatches[0].model->Unbind();
	UnbindModel();
	glEnable(GL_CULL_FACE);
	//points draw
	if(need_points)
	{
		glDisable(GL_DEPTH_TEST);
		g_progsManager.SelectProg(eProgType_Color,0);
		g_progsManager.SetMtx(mapMtx);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		for(int i=0; i<numStaticPropsBatches; i++)
		{
			if(!staticPropsBatches[i].model)
			{
				if(i==0)
					g_progsManager.SetColor(0.5f,1.0f,1.0f,1.0f);
				else
					g_progsManager.SetColor(0.5f,0.5f,1.0f,1.0f);

				glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,(float*)staticPropsBatches[i].positions);
				if(log_render)
					LOG("glDrawArrays %d DrawStaticPropsOpaque\n", staticPropsBatches[i].numInsts);
				glDrawArrays(GL_POINTS,0,staticPropsBatches[i].numInsts);
			}
		}
		glEnable(GL_DEPTH_TEST);
	}
	glCullFace(GL_BACK);

	CheckGLError("DrawStaticProps opaque", __FILE__, __LINE__);
}

void map_t::UnbindModel(){
	glCullFace(GL_BACK);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

