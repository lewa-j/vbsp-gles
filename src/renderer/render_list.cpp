
#include "log.h"
#include "graphics/platform_gl.h"
#include "renderer/progs_manager.h"
#include "renderer/render_list.h"

using namespace std;

void RenderList::SetMtx(glm::mat4 mtx)
{
	modelMtx = mtx;
}

void RenderList::Clean()
{
	for(map<glslProg*, map<Material*, queue<DrawCall> > >::iterator it = data.begin(); it!=data.end(); it++)
	{
		//for(map<Material*, queue<DrawCall>>::iterator mit = it->second.begin(); mit!=it->second.end(); mit++)
		//{}
		it->second.clear();
	}
	data.clear();

	for(map<glslProg*, map<Material*, std::map<model_t*, queue<DrawCall> > > >::iterator it = modelsData.begin(); it!=modelsData.end(); it++)
	{
		for(map<Material*, std::map<model_t*, queue<DrawCall> > >::iterator mit = it->second.begin(); mit!=it->second.end(); mit++)
		{
			mit->second.clear();
		}
		it->second.clear();
	}
	modelsData.clear();
}

void RenderList::Add(Material *m, submesh_t sm, int flags)
{
	glslProg *cProg = g_progsManager.GetProg(m->program,flags|m->flags);
	queue<DrawCall> *q = &data[cProg][m];
	DrawCall d;
	d.sm = sm;
	q->push(d);
}

void RenderList::Add(model_t *md, glm::mat4 *mtx, glm::vec3 *light, int flags)
{
	for(unsigned int k = 0; k < md->numMeshes; k++)
	{
		if(md->materials[k]->program==eProgType_Refract)
			continue;
		glslProg *cProg = g_progsManager.GetProg(md->materials[k]->program,flags|md->materials[k]->flags);
		queue<DrawCall> *q = &modelsData[cProg][md->materials[k]][md];
		DrawCall d;
		d.sm = md->indsOfs[k];
		d.modelMtx = mtx;
		d.light = light;
		q->push(d);
	}
}

void RenderList::Flush()
{
	for(map<glslProg*, map<Material*, queue<DrawCall> > >::iterator it = data.begin(); it!=data.end(); it++)
	{
		g_progsManager.SetProg(it->first);
		g_progsManager.SetMtx(modelMtx);
		for(map<Material*, queue<DrawCall> >::iterator mit = it->second.begin(); mit!=it->second.end(); mit++)
		{
			mit->first->Bind();
			queue<DrawCall> *q = &mit->second;
			while(!q->empty())
			{
				submesh_t sm = q->front().sm;
				if(log_render)
					LOG("glDrawElements %d %d RenderList\n", sm.Count, sm.Offset);
				glDrawElements(GL_TRIANGLES, sm.Count, GL_UNSIGNED_INT, (void *)(sm.Offset*sizeof(int)));
				q->pop();
			}
		}
	}
}

void RenderList::FlushModels()//bool transp)
{
	if(log_render)
		LOG("RenderList::FlushModels\n");
	//std::map<glslProg*, std::map<Material*, std::map<model_t*, std::queue<DrawCall> > > > modelsData;
	for(map<glslProg*, map<Material*, std::map<model_t*, queue<DrawCall> > > >::iterator it = modelsData.begin(); it!=modelsData.end(); it++)
	{
		g_progsManager.SetProg(it->first);
		if(log_render)
			LOG("RenderList::FlushModels SetProg %p\n", it->first);
		for(map<Material*, std::map<model_t*, queue<DrawCall> > >::iterator mit = it->second.begin(); mit!=it->second.end(); mit++)
		{
			Material* mat = mit->first;
			if(log_render)
				LOG("RenderList::FlushModels mat->Bind() %p\n", mat);
			if(mat->transparent||mat->additive)
				continue;
			mat->Bind();
			//if(log_render)
			//	LOG("RenderList::FlushModels mat->Bind() %p\n", mat);
			if(mat->nocull)
				glDisable(GL_CULL_FACE);
			for(std::map<model_t*, queue<DrawCall> >::iterator mdit = mit->second.begin(); mdit !=mit->second.end(); mdit++)
			{
				mdit->first->Bind();
				queue<DrawCall> *q = &mdit->second;
				while(!q->empty())
				{
					g_progsManager.SetMtx(*q->front().modelMtx);
					g_progsManager.SetLeafAmbient(q->front().light);
					submesh_t sm = q->front().sm;

					if(log_render)
						LOG("glDrawElements %d %d RenderList::FlushModels\n", sm.Count, sm.Offset*sizeof(int));
					glDrawElements(GL_TRIANGLES, sm.Count, GL_UNSIGNED_INT, (void*)(sm.Offset*sizeof(int)));

					q->pop();
				}
			}
			if(mat->nocull)
				glEnable(GL_CULL_FACE);
		}
	}

//TODO move it before transparent world
	//transparrent
	glDepthMask(0);
	for(map<glslProg*, map<Material*, std::map<model_t*, queue<DrawCall> > > >::iterator it = modelsData.begin(); it!=modelsData.end(); it++)
	{
		g_progsManager.SetProg(it->first);
		for(map<Material*, std::map<model_t*, queue<DrawCall> > >::iterator mit = it->second.begin(); mit!=it->second.end(); mit++)
		{
			Material* mat = mit->first;
			if(!mat->transparent&&!mat->additive)
				continue;
			mat->Bind();
			if(mat->transparent||mat->additive)
			{

				glEnable(GL_BLEND);
				if(mat->additive)
					glBlendFunc(1,1);
				else
					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			}
			if(mat->nocull)
				glDisable(GL_CULL_FACE);
			for(std::map<model_t*, queue<DrawCall> >::iterator mdit = mit->second.begin(); mdit !=mit->second.end(); mdit++)
			{
				mdit->first->Bind();
				queue<DrawCall> *q = &mdit->second;
				while(!q->empty())
				{
					g_progsManager.SetMtx(*q->front().modelMtx);
					g_progsManager.SetLeafAmbient(q->front().light);
					submesh_t sm = q->front().sm;

					if(log_render)
						LOG("glDrawElements %d %d RenderList::FlushModels\n", sm.Count, sm.Offset*sizeof(int));
					glDrawElements(GL_TRIANGLES, sm.Count, GL_UNSIGNED_INT, (void*)(sm.Offset*sizeof(int)));

					q->pop();
				}
			}
			if(mat->transparent||mat->additive)
			{
				glDisable(GL_BLEND);
			}
			if(mat->nocull)
				glEnable(GL_CULL_FACE);
		}
	}
	glDepthMask(1);
}
