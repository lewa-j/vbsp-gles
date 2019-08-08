
#include "graphics/platform_gl.h"

#include "engine.h"
#include "log.h"
#include "world.h"
#include "physics.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"

int map_t::ConvertEdgeToIndex(int edge)
{
	int e = surfedges[edge];
	return (e > 0) ? edges[e].v[0] : edges[-e].v[1];
}

void map_t::InitPhysics()
{
	//LOG("Init bsp map physics\n");
	//TODO brushes

	//faces
	if(mapmesh.numVerts>65000){
		EngineError("Init map physics: too many verts");
		return;
	}

	int *inds = new int[model->numInds];
	//uint16_t *inds = new uint16_t[model->numInds];
	unsigned int curInd=0;
	for(int i = models[0].firstface; i < models[0].firstface+models[0].numfaces; i++)
	{
		bsptexinfo_t ti = texInfos[faces[i].texinfo];
		if((ti.flags&SURF_SKY)||(faces[i].dispinfo!=-1))
		{
			continue;
		}
		int f = faces[i].firstedge;
		for(int j=0; j<faces[i].numedges-2; j++)
		{
			inds[curInd++] = ConvertEdgeToIndex(f);
			inds[curInd++] = ConvertEdgeToIndex(f+j+1);
			inds[curInd++] = ConvertEdgeToIndex(f+j+2);
		}
	}
	if(model->numInds!=curInd)
		EngineError("InitPhysics model->numInds!=curInd");

	LOG("Init world physics: %d verts %d tris\n",mapmesh.numVerts,curInd/3);
	btIndexedMesh msh;
	msh.m_vertexBase = (uint8_t*)mapmesh.verts;
	msh.m_vertexStride = sizeof(float)*3;
	msh.m_numVertices = mapmesh.numVerts;
	msh.m_triangleIndexBase = (uint8_t*)inds;
	msh.m_numTriangles = curInd/3;
	//int
	msh.m_triangleIndexStride = sizeof(int)*3;
	msh.m_indexType = PHY_INTEGER;
	//short
	//msh.m_triangleIndexStride = sizeof(uint16_t)*3;
	//msh.m_indexType = PHY_SHORT;

	btTriangleIndexVertexArray *vertArray = new btTriangleIndexVertexArray();
	vertArray->addIndexedMesh(msh);

	btBvhTriangleMeshShape *shape = new btBvhTriangleMeshShape(vertArray,true);
	shape->setLocalScaling(btVector3(0.026,0.026,0.026));
	btTransform trans;
	trans.setIdentity();
	trans.setRotation(btQuaternion(0,btRadians(-90),0));
	//trans.setFromOpenGLMatrix((float*)&mapMtx);
	g_physics.CreateRigidBody(0,trans,shape);


	//displacements
	if(numDispBatches){
		int numDispVerts = 0;
		int numDispInds = 0;
		for(int i=0; i<numDispBatches; i++){
			numDispVerts += dispBatches[i].numVerts;
			numDispInds += dispBatches[i].submesh.Count;
		}
		uint8_t *physDispVerts = new uint8_t[numDispVerts*12];
		uint8_t *physDispInds = new uint8_t[numDispInds*4];
		glm::vec3 *curVert=(glm::vec3*)physDispVerts;
		int curIndOffs=0;
		int *curInd = (int*)physDispInds;
		for(int i=0; i<numDispBatches; i++){
			for(uint32_t v=0; v<dispBatches[i].numVerts;v++){
				curVert[v]=((dispvert_t*)dispBatches[i].verts)[v].pos;
			}
			for(uint32_t j=0;j<dispBatches[i].submesh.Count;j++){
				curInd[j] = curIndOffs+((int*)dispBatches[i].submesh.Offset)[j];
			}
			curInd += dispBatches[i].submesh.Count;
			curVert += dispBatches[i].numVerts;
			curIndOffs += dispBatches[i].numVerts;
		}

		btIndexedMesh dmsh;
		dmsh.m_vertexBase = physDispVerts;
		dmsh.m_vertexStride = 12;
		dmsh.m_numVertices = numDispVerts;
		dmsh.m_triangleIndexBase = physDispInds;
		dmsh.m_numTriangles = numDispInds/3;
		dmsh.m_triangleIndexStride = sizeof(int)*3;
		dmsh.m_indexType = PHY_INTEGER;

		btTriangleIndexVertexArray *dvertArray = new btTriangleIndexVertexArray();
		dvertArray->addIndexedMesh(dmsh);

		btBvhTriangleMeshShape *dshape = new btBvhTriangleMeshShape(dvertArray,true);
		dshape->setLocalScaling(btVector3(0.026,0.026,0.026));
		trans.setIdentity();
		trans.setRotation(btQuaternion(0,btRadians(-90),0));
		g_physics.CreateRigidBody(0,trans,dshape);
	}
}
