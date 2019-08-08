
#include "graphics/platform_gl.h"
#include "world.h"
#include "renderer/render_vars.h"

mleaf_t *cameraLeaf = 0;
mleaf_t *oldCameraLeaf = 0;
int curVisFrame = 0;
int curframe = 0;

int renderLeafs = 0;
extern Camera camera;
extern glm::vec3 curViewPos;

bool map_t::TestVisLeaf(int l, mleaf_t *vl)
{
	if(leafs[l].cluster==-1)
		return false;
	if(vl->cluster==-1)
		return false;
	return TestVisCluster(leafs[l].cluster,vl->cluster);
}

bool map_t::TestVisCluster(int c, int vc)
{
	return (visClustersData[c*numVisClusters+vc]);
}

mleaf_t *map_t::TraceTree(glm::vec3 pos)
{
	mnode_t *n = nodes + models[0].headnode;
	bspplane_t p;

	while(1)
	{
		if(n->contents>=0)
			return (mleaf_t *)n;
		p = *(n->plane);

		n = n->children[(glm::dot(p.normal, pos) - p.dist <= 0)];
	}

	return NULL;
}

void map_t::MarkLeaves()
{
	if(cameraLeaf == oldCameraLeaf && !no_vis && cameraLeaf->cluster != -1)
		return;

	curVisFrame++;
	oldCameraLeaf = cameraLeaf;

	if(no_vis || cameraLeaf->cluster == -1)
	{
		for(int i=0; i<numLeafs; i++)
		{
			leafs[i].visFrame = curVisFrame;
		}
		for(int i=0; i<numNodes; i++)
		{
			nodes[i].visFrame = curVisFrame;
		}
		return;
	}

	for(int i=0; i<numLeafs; i++)
	{
		if(no_vis || TestVisLeaf(i,cameraLeaf))
		{
			mnode_t *n = (mnode_t *)(leafs+i);
			while(1)
			{
				if(n->visFrame == curVisFrame)
					break;
				n->visFrame = curVisFrame;
				n = n->parent;
				if(!n)
					break;
			}
		}
	}
}

void map_t::RecursiveWorldNode (mnode_t *node)
{
	if(node->contents == 1)//solid
		return;

	if(node->visFrame != curVisFrame)
		return;

	if(!no_cull && !camera.frustum.Contains(node->mins,node->maxs))
		return;

	if(node->contents != -1)
	{
		mleaf_t *l=(mleaf_t*)node;

		//TODO: Areas
		//if(!no_vis && cameraLeaf->area != 0 && l->area!=cameraLeaf->area)
		//	return;

		for(int i = 0;i<l->numLeafFaces;i++)
		{
			surfaces[l->leafFaces[i]].visframe = curframe;
		}
		renderLeafs++;

		return;
	}


	bspplane_t p = *(node->plane);
	int side = (glm::dot(p.normal, curViewPos) - p.dist <= 0);

	RecursiveWorldNode(node->children[side]);
	RecursiveWorldNode(node->children[!side]);
}

