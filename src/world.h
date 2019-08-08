
#pragma once

#include <vector>
#include <string>
#include <bitset>

#include <vec2.hpp>
#include "graphics/ArrayBuffer.h"
#include "cull/BoundingBox.h"
#include "system/neZip.h"

#include "renderer/vbsp_mesh.h"
#include "renderer/vbsp_material.h"
#include "file_format/bsp.h"
#include "file_format/mdl.h"

class Entity
{
public:
	glm::vec3 origin;
	glm::vec3 angles;
	std::string targetname;
	std::string classname;
	int spawnflags;
	
	Entity()
	{
		origin = glm::vec3(0);
		angles = glm::vec3(0);
		targetname = "";
		classname = "";
		spawnflags = 0;
	}

	~Entity()
	{
		targetname.clear();
		classname.clear();
	}
	
	virtual void Spawn()
	{
		
	}
};

struct worldvert_t
{
	glm::vec3 pos;//0
	glm::vec3 normal;//3
	glm::vec2 uv;//6
	glm::vec4 lmuv;//8 xy: coord zw: offset
	glm::vec3 tangent;
};

struct dispvert_t
{
	glm::vec3 pos;
	float alpha;
	glm::vec2 uv;
	glm::vec2 lmuv;
};

struct mnode_t
{
	int contents;
	int visFrame;
	mnode_t *parent;
	glm::vec3 mins;
	glm::vec3 maxs;
	short area;

	bspplane_t *plane;
	mnode_t *children[2];
};

struct mleaf_t
{
	int contents;
	int visFrame;
	mnode_t *parent;
	glm::vec3 mins;
	glm::vec3 maxs;
	short area;

	short cluster;
	uint16_t *leafFaces;
	uint16_t numLeafFaces;
	bspleafambientlighting_t *ambientLight;
	glm::vec3 ambientLightTemp[6];
};

struct mface_t
{
	int visframe;
	int offset;
	int normalOffset;
	int count;
};

struct materialBatch_t
{
	Material *material;
	int lightmap;
	int numFaces;
	int *faces;
	//int numInds;
	//int indOffset;
	submesh_t submesh;
	GLfloat *verts;
	GLuint numVerts;
	int frame;
	
	materialBatch_t()
	{
		material = 0;
		lightmap = -1;
		numFaces = 0;
		faces = 0;
		//numInds = 0;
		//indOffset = 0;
		verts = 0;
		numVerts = 0;
		frame=-1;
	}

	void Draw();
};

struct leafSegment_t
{
	uint16_t *leafs;
	int count;
	
	leafSegment_t()
	{
		leafs=0;
		count=0;
	}
	
	leafSegment_t(uint16_t *l, int c)
	{
		leafs=l;
		count=c;
	}
};

struct staticPropInst_t
{
	glm::mat4 modelMtx;
	leafSegment_t leafs;
	int visFrame;
	mleaf_t *lightLeaf;
};

struct staticPropsBatch_t
{
	model_t *model;
	glm::vec3 *positions;
	staticPropInst_t *insts;
	int numInsts;
	float dist;
	int visFrame;
};

extern mleaf_t *cameraLeaf;
extern mleaf_t *oldCameraLeaf;
extern int curVisFrame;
extern int curframe;

extern int renderMats;
extern int renderLeafs;
extern int renderProps;

struct map_t
{
	map_t();
	~map_t();

	std::string name;
	bool loaded;
	Mesh mapmesh;
	Mesh *model;
	VertexBufferObject modelVbo;
	IndexBufferObject modelIbo;

	std::vector<Entity> entities;
	glm::vec3 *entPos;
	int numEnts;
	bspplane_t *planes;
	int numPlanes;
	bsptexdata_t *texdata;
	int numTexdata;
	//vis
	uint8_t *visClustersData;
	int numVisClusters;
	bsptexinfo_t *texInfos;
	int numTexInfos;
	bspface_t *faces;
	int numFaces;

	mnode_t *nodes;
	int numNodes;
	mleaf_t *leafs;
	int numLeafs;
	uint16_t *leafFaces;
	int numLeafFaces;

	bspedge_t *edges;
	int numEdges;
	int *surfedges;
	int numSurfedges;
	bspmodel_t *models;
	int numModels;
	bspareaportal_t *areaPortals;
	int numAreaPortals;
	glm::vec3 *areaPortalVerts;
	int numAreaPortalVers;
	bspdispinfo_t *dispInfos;
	int numDispInfos;
	bspcubemapsample_t *cubemapSamples;
	int numCubemaps;
	bspleafambientindex_t *ambientInds;
	int numAmbientInds;
	bspleafambientlighting_t *ambientLights;
	int numAmbientLights;

	//temp
	int *texdataStringTable;
	char *texdataString;
	glm::vec3 *vertNormals;
	int numVertNormals;
	uint16_t *vertNormalIndices;
	int numVertNormalIndices;
	int leafLumpVersion;
	
	mface_t *surfaces;
	int numOpaqueBatches;
	materialBatch_t *opaqueBatches;
	int numTransparentBatches;
	materialBatch_t *transparentBatches;
	//todo: disp batches
	int numDispBatches;
	materialBatch_t *dispBatches;
	int numStaticPropsBatches;
	staticPropsBatch_t *staticPropsBatches;
	uint16_t *propsLeafs;

	zipArchive pak;

	
	//bsp_loader.cpp
	bool Load(const char *filename);
	void InitEnts();
	//bsp_renderer.cpp
	void Draw();
	//bsp_cull.cpp
	bool TestVisLeaf(int l, mleaf_t *vl);
	bool TestVisCluster(int c, int vc);
	mleaf_t *TraceTree(glm::vec3 pos);
	void MarkLeaves();
	void RecursiveWorldNode (mnode_t *node);
	//bsp_physics.cpp
	void InitPhysics();
private:
	void DrawEntities();
	void DrawStaticPropsOpaque();
	void DrawStaticPropsTransparent();
	void BindBSP();

	void LoadLeafs(IFile *in, bsplump_t lump);
	void LoadNodes(IFile *in, bsplump_t lump);
	void SetupLeafsNodes();
	void LoadVis(IFile *in, bsplump_t lump);
	void LoadDisplacements(IFile *in, bsplump_t ilump, bsplump_t vlump, CDispVert **dispVerts);
	void LoadGameLump(IFile *in, bsplump_t lump);
	void LoadStaticProps(IFile *in, bspgamelump_t lump);
	void LoadTextures(IFile *in, bspheader_t &header);
	void LoadAreaPortals(IFile *in, bsplump_t alump, bsplump_t plump, bsplump_t vlump);
	void LoadPaklump(IFile *in, bsplump_t lump);
	void LoadCubemaps(IFile *in, bsplump_t lump);
	void LoadVertNormals(IFile *in, bsplump_t lumpn, bsplump_t lumpi);
	void LoadAmbientLighting(IFile *in, bsplump_t lumpi, bsplump_t lumpl);

	void CreateModel(int m, ColorRGBExp32 *samples);
	void BuildIndices();
	void CreateFaceSubmesh(Mesh *mod, mface_t *surface, ColorRGBExp32 *samples, int i);
	void CreateDisp(int i, ColorRGBExp32 *samples, CDispVert *dispVerts);
	void ParseEntities(char *data);
	int ConvertEdgeToIndex(int edge);

	void UnbindModel();
};

