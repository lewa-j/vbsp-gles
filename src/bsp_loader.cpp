
#include <fstream>
#include <cstring>
#include <cstdlib>
using namespace std;

#include <glm.hpp>

#include "engine.h"
#include "log.h"
#include "system/FileSystem.h"

#include "graphics/platform_gl.h"
#include "graphics/gl_utils.h"
#include "graphics/texture.h"

#include "renderer/render_vars.h"
#include "renderer/vbsp_mesh.h"
#include "file_format/bsp.h"
#include "renderer/vbsp_material.h"
#include "file_format/mdl.h"
#include "world.h"
#include "vbsp.h"

glm::vec3 RGBExpToRGB(ColorRGBExp32 s);

void LM_InitBlock();
int LM_AllocBlock(int w, int h, GLuint *x, GLuint *y);
void LM_UploadBlock();

#define BLOCK_SIZE 2048
//512
GLuint allocated[BLOCK_SIZE];
GLubyte *lightmap_buffer;
GLuint current_lightmap_texture = 0;
Texture g_lightmaps[8];

bool load_prop = 1;
bool hdr = false;

map_t::map_t():pak()
{
	loaded = false;
}

map_t::~map_t()
{
	if(pak.data)
		delete[] pak.data;
}

bool map_t::Load(const char *fileName)
{
	name = fileName;
	double startTime=GetTime();
	string filePath = "maps/"+name+".bsp";
	IFile *in = g_fs.Open(filePath.c_str());
	if(!in){
		//LOG( "File: %s not found\n",fileName);
		return false;
	}
	loaded = false;
	Log( "Loading map: %s\n",fileName);

	bspheader_t header;
	in->Read(&header,sizeof(bspheader_t));

	if(header.ident != BSPIDENT){
		Log("header ident %d\n",header.ident);
		g_fs.Close(in);
		EngineError( "Unknown map format");
		return false;
	}

	Log( "VBSP version %d\n", header.version);
//========================================================================================================
	char* entities = new char[header.lumps[LUMP_ENTITIES].filelen];

	in->Seek(header.lumps[LUMP_ENTITIES].fileofs);
	in->Read(entities,header.lumps[LUMP_ENTITIES].filelen);

	//LOG("%s\n",entities);
	filePath = "maps/"+name+".ent";
	char path[1024];
	if(!g_fs.GetFilePath(filePath.c_str(), path, true)){
		ofstream ent_out(path, ios::binary);
		ent_out << entities;
		ent_out.close();
	}

	ParseEntities(entities);

	delete[] entities;

//========================================================================================================

	numPlanes = header.lumps[LUMP_PLANES].filelen/sizeof(bspplane_t);
	Log( "Loading %d planes\n",numPlanes);
	planes = new bspplane_t[numPlanes];
	in->Seek(header.lumps[LUMP_PLANES].fileofs);
	in->Read(planes,header.lumps[LUMP_PLANES].filelen);

//========================================================================================================
	int numVerts = header.lumps[LUMP_VERTEXES].filelen/sizeof(glm::vec3);
	Log( "Loading %d vertices\n",numVerts);
	glm::vec3 *vertices = new glm::vec3[numVerts];
	in->Seek(header.lumps[LUMP_VERTEXES].fileofs);
	in->Read(vertices,header.lumps[LUMP_VERTEXES].filelen);

//========================================================================================================
	LoadVis(in, header.lumps[LUMP_VISIBILITY]);

//========================================================================================================
	numTexInfos = header.lumps[LUMP_TEXINFO].filelen/sizeof(bsptexinfo_t);
	Log( "Loading %d texInfos\n", numTexInfos);
	texInfos = new bsptexinfo_t[numTexInfos];
	in->Seek(header.lumps[LUMP_TEXINFO].fileofs);
	in->Read(texInfos, header.lumps[LUMP_TEXINFO].filelen);

//========================================================================================================
	int faces_lump = LUMP_FACES;
	int lump_lighting = LUMP_LIGHTING;
	int lump_leafLightInd = LUMP_LEAF_AMBIENT_INDEX;
	int lump_leafLight = LUMP_LEAF_AMBIENT_LIGHTING;
	if(!header.lumps[lump_leafLightInd].filelen){
		if(!header.lumps[LUMP_LEAF_AMBIENT_INDEX_HDR].filelen)
			Log("LUMP_LEAF_AMBIENT_INDEX length is zero\n");
		else{
			lump_leafLightInd = LUMP_LEAF_AMBIENT_INDEX_HDR;
			lump_leafLight = LUMP_LEAF_AMBIENT_LIGHTING_HDR;
		}
	}
	if(!header.lumps[LUMP_LIGHTING].filelen){
		if(header.lumps[LUMP_LIGHTING_HDR].filelen){
			faces_lump = LUMP_FACES_HDR;
			lump_lighting = LUMP_LIGHTING_HDR;
			lump_leafLightInd = LUMP_LEAF_AMBIENT_INDEX_HDR;
			lump_leafLight = LUMP_LEAF_AMBIENT_LIGHTING_HDR;
			hdr = true;
			Log("Loading HDR lighting\n");
		}
	}

	numFaces = header.lumps[faces_lump].filelen/sizeof(bspface_t);
	Log( "Loading %d faces\n", numFaces );
	faces = new bspface_t[numFaces];

	in->Seek(header.lumps[faces_lump].fileofs);
	in->Read(faces,header.lumps[faces_lump].filelen);

//========================================================================================================
	int numLightmapSamples = header.lumps[lump_lighting].filelen/sizeof(ColorRGBExp32);
	Log( "Loading %d lightmap samples\n", numLightmapSamples );
	ColorRGBExp32 *samples = new ColorRGBExp32[numLightmapSamples];

	in->Seek(header.lumps[lump_lighting].fileofs);
	in->Read(samples,header.lumps[lump_lighting].filelen);

//========================================================================================================
	numLeafFaces = header.lumps[LUMP_LEAFFACES].filelen/sizeof(uint16_t);
	leafFaces = new uint16_t[numLeafFaces];
	in->Seek(header.lumps[LUMP_LEAFFACES].fileofs);
	in->Read(leafFaces,header.lumps[LUMP_LEAFFACES].filelen);

	LoadLeafs(in, header.lumps[LUMP_LEAFS]);
	LoadNodes(in, header.lumps[LUMP_NODES]);
	LoadAmbientLighting(in,header.lumps[lump_leafLightInd],header.lumps[lump_leafLight]);

	SetupLeafsNodes();

//========================================================================================================
	numEdges = header.lumps[LUMP_EDGES].filelen/sizeof(bspedge_t);
	Log( "Loading %d edges\n",numEdges);
	edges = new bspedge_t[numEdges];
	in->Seek(header.lumps[LUMP_EDGES].fileofs);
	in->Read(edges,header.lumps[LUMP_EDGES].filelen);

//========================================================================================================
	numSurfedges = header.lumps[LUMP_SURFEDGES].filelen/sizeof(int);
	Log( "Loading %d surfedges\n",numSurfedges);
	surfedges = new int[numSurfedges];
	in->Seek(header.lumps[LUMP_SURFEDGES].fileofs);
	in->Read(surfedges,header.lumps[LUMP_SURFEDGES].filelen);

//========================================================================================================
	numModels = header.lumps[LUMP_MODELS].filelen/sizeof(bspmodel_t);
	Log( "Loading %d models\n",numModels);
	models = new bspmodel_t[numModels];
	in->Seek(header.lumps[LUMP_MODELS].fileofs);
	in->Read(models,header.lumps[LUMP_MODELS].filelen);

//========================================================================================================

	LoadAreaPortals(in,header.lumps[LUMP_AREAS],header.lumps[LUMP_AREAPORTALS],header.lumps[LUMP_CLIPPORTALVERTS]);

	//LOG("occulusion len %d\n", header.lumps[LUMP_OCCLUSION].filelen);
	//LOG("world lights len %d\n", header.lumps[LUMP_WORLDLIGHTS].filelen);
	//LOG("overlays len %d\n", header.lumps[LUMP_OVERLAYS].filelen);
//========================================================================================================

	LoadVertNormals(in,header.lumps[LUMP_VERTNORMALS],header.lumps[LUMP_VERTNORMALINDICES]);

	LoadPaklump(in,header.lumps[LUMP_PAKFILE]);

	LoadCubemaps(in,header.lumps[LUMP_CUBEMAPS]);

//========================================================================================================
	CDispVert *dispVerts=0;
	LoadDisplacements(in, header.lumps[LUMP_DISPINFO], header.lumps[LUMP_DISP_VERTS], &dispVerts);
	
	LoadGameLump(in, header.lumps[LUMP_GAME_LUMP]);
	
	LoadTextures(in, header);

	if(header.lumps[LUMP_MAP_FLAGS].filelen){
		int mapflags = 0;
		in->Seek(header.lumps[LUMP_MAP_FLAGS].fileofs);
		in->Read(&mapflags, header.lumps[LUMP_MAP_FLAGS].filelen);
		Log("map flags %d\n", mapflags);
	}
//============

	mapmesh = Mesh((float*)vertices, numVerts, GL_POINTS);

	lightmap_buffer = new GLubyte[BLOCK_SIZE*BLOCK_SIZE*3];
	LM_InitBlock();
	
	CreateModel(0, samples);

	Log("Creating displacements\n");
	numDispBatches = numDispInfos;
	dispBatches = new materialBatch_t[numDispBatches];
	for(int i=0; i<numDispInfos; i++){
		CreateDisp(i, samples, dispVerts);
	}

	Log("Loaded %d materials\n", g_MaterialManager.loadedMaterials.size());

	LM_UploadBlock();
	delete[] lightmap_buffer;

	delete[] dispVerts;
	//delete[] vertices;
	//delete[] faces;
	delete[] samples;
	//delete[] edges;
	//delete[] surfedges;
	delete[] texdataStringTable;
	delete[] texdataString;
	delete[] vertNormals;
	delete[] vertNormalIndices;

	g_fs.Close(in);
	
	Log("Map %s loaded (%f s)\n",fileName,GetTime()-startTime);
	loaded = true;
	return true;
}

#if 1
void map_t::CreateModel(int m, ColorRGBExp32 *samples)
{
	model = new Mesh();
	surfaces = new mface_t[models[m].numfaces];
	int i = 0;
	int nrmoffs=0;
	for(i = models[m].firstface; i < models[m].firstface+models[m].numfaces; i++){
		nrmoffs += faces[i].numedges;
		if((texInfos[faces[i].texinfo].flags&SURF_SKY)||(faces[i].dispinfo!=-1)){
			//LOG("Skip face %d\n",i);
			continue;
		}
		surfaces[i].offset = model->numVerts;
		surfaces[i].normalOffset = nrmoffs - faces[i].numedges;
		surfaces[i].count = faces[i].numedges;
		model->numVerts += faces[i].numedges;
		model->numInds += (faces[i].numedges-2)*3;
	}
	model->verts = (float*)new worldvert_t[model->numVerts];
	model->inds = new GLuint[model->numInds];
	Log("Generating model %d: %d verts and %d inds\n", m, model->numVerts, model->numInds);


	numOpaqueBatches=0;
	numTransparentBatches=0;
	//LOG("numTexdata %d\n",numTexdata);
	vector<int> *materialFaces = new vector<int>[numTexdata];
	//materialInds = new submesh_t[numTexdata];
	for(i = models[m].firstface; i < models[m].firstface+models[m].numfaces; i++){
		bsptexinfo_t ti = texInfos[faces[i].texinfo];
		int curMat = ti.texdata;
		if((ti.flags&SURF_SKY)||(faces[i].dispinfo!=-1)){
			continue;
		}
		materialFaces[curMat].push_back(i);
	}

	vector<materialBatch_t> tempOpBatches;
	vector<materialBatch_t> tempTrBatches;

	for(i = 0; i<numTexdata; i++){
		char *matName = texdataString+texdataStringTable[texdata[i].nameStringTableID];
		//LOG("texdata %d %d %s\n", i, materialFaces[i].size(), matName);
		if(materialFaces[i].empty())
			continue;

		//char *matName = texdataString+texdataStringTable[texdata[i].nameStringTableID];
		//LOG("texdata %d %s\n",i,matName);
		Material *mat = g_MaterialManager.GetMaterial(matName);
		if(!mat)
			mat = g_MaterialManager.GetMaterial("*");
		/*if(!mat.transparent)
			numOpaqueBatches++;
		else
			numTransparentBatches++;*/
		materialBatch_t tempBatch;
		ColorRGBExp32 *ls = samples;
		if(mat->program == eProgType_Unlit){
			ls = 0;
		}else{
			tempBatch.lightmap = current_lightmap_texture;
		}
		tempBatch.material = mat;
		tempBatch.numFaces = materialFaces[i].size();
		tempBatch.faces = new int[tempBatch.numFaces];
		memcpy(tempBatch.faces, &materialFaces[i][0], tempBatch.numFaces*sizeof(int));

		for(GLuint j=0; j<materialFaces[i].size(); j++){
			int f = materialFaces[i][j];
			CreateFaceSubmesh(model, &surfaces[f],ls,f);
		}

		//LOG("matBatch %d ind ofs %d num %d\n", i, tempBatch.indOffset, tempBatch.numInds);
		if(!mat->transparent){
			tempOpBatches.push_back(tempBatch);
		}else{
			tempTrBatches.push_back(tempBatch);
		}
	}

	numOpaqueBatches = tempOpBatches.size();
	numTransparentBatches = tempTrBatches.size();
	Log("Generate batches: %d opaque, %d transparent\n", numOpaqueBatches, numTransparentBatches);
	opaqueBatches = new materialBatch_t[numOpaqueBatches];
	transparentBatches = new materialBatch_t[numTransparentBatches];
	memcpy(opaqueBatches,&tempOpBatches[0], numOpaqueBatches*sizeof(materialBatch_t));
	memcpy(transparentBatches, &tempTrBatches[0], numTransparentBatches*sizeof(materialBatch_t));

	delete[] materialFaces;

	pthread_mutex_lock(&graphicsMutex);

	modelVbo.Create();
	modelVbo.Upload(model->numVerts*sizeof(worldvert_t), model->verts);
	delete[] model->verts;
	model->verts=0;

	modelIbo.Create();
	modelIbo.Upload(model->numInds*sizeof(int), 0, GL_DYNAMIC_DRAW);
	BuildIndices();
	//modelIbo.Upload(model->numInds*4, model->inds);
	//delete[] model->inds;
	//model->inds=0;

	pthread_mutex_unlock(&graphicsMutex);
}
#endif

void map_t::BuildIndices()
{
	GLuint *dest = model->inds;
	int curInd = 0;

	for(int i =0; i<numOpaqueBatches;i++){
		opaqueBatches[i].submesh.Offset = curInd;
		opaqueBatches[i].submesh.Count = 0;
		for(int j=0; j<opaqueBatches[i].numFaces; j++){
			int f = opaqueBatches[i].faces[j];

			if(surfaces[f].visframe!=curframe)
				continue;
			//TODO: cull

			int v = surfaces[f].offset;

			opaqueBatches[i].submesh.Count += (surfaces[f].count-2)*3;
			for(int k = 0;k<(int)surfaces[f].count-2;k++){
				dest[0] = v;
				dest[1] = v+k+2;
				dest[2] = v+k+1;
				dest+=3;
			}
		}
		curInd += opaqueBatches[i].submesh.Count;
	}

	for(int i =0; i<numTransparentBatches;i++){
		transparentBatches[i].submesh.Offset = curInd;
		transparentBatches[i].submesh.Count = 0;
		for(int j=0; j<transparentBatches[i].numFaces; j++){
			int f = transparentBatches[i].faces[j];

			if(surfaces[f].visframe!=curframe)
				continue;
			//TODO: cull

			int v = surfaces[f].offset;

			transparentBatches[i].submesh.Count += (surfaces[f].count-2)*3;
			for(int k = 0;k<(int)surfaces[f].count-2;k++){
				dest[0] = v;
				dest[1] = v+k+2;
				dest[2] = v+k+1;
				dest+=3;
			}
		}
		curInd += transparentBatches[i].submesh.Count;
	}

	if(curInd>(int)model->numInds){
		Log("curInd %d model->numInds %d\n", curInd, model->numInds);
		EngineError("curInds>model->numInds");
	}
	modelIbo.Update(0,curInd*sizeof(int), model->inds);
	if(log_render)
		Log("Generated %d inds\n", curInd);
}

void map_t::LoadLeafs(IFile *in, bsplump_t lump)
{
	leafLumpVersion = lump.version;
	if(lump.version==0)
	{
		numLeafs = lump.filelen/sizeof(bspleaf_version_0_t);
		LOG("Loading %d leafs (v.%d)\n", numLeafs,lump.version);
		bspleaf_version_0_t *fleafs = new bspleaf_version_0_t[numLeafs];
		in->Seek(lump.fileofs);
		in->Read(fleafs,lump.filelen);
		leafs = new mleaf_t[numLeafs];
		memset(leafs,0,sizeof(mleaf_t)*numLeafs);
		for(int i=0; i<numLeafs; i++)
		{
			leafs[i].contents = fleafs[i].contents;
			leafs[i].cluster = fleafs[i].cluster;
			leafs[i].area = fleafs[i].area;
			leafs[i].mins = glm::mat3(mapMtx)*glm::vec3(fleafs[i].mins[0],fleafs[i].mins[1],fleafs[i].mins[2]);
			leafs[i].maxs = glm::mat3(mapMtx)*glm::vec3(fleafs[i].maxs[0],fleafs[i].maxs[1],fleafs[i].maxs[2]);
			leafs[i].numLeafFaces = fleafs[i].numleaffaces;
			leafs[i].leafFaces = leafFaces + fleafs[i].firstleafface;
			for(int j = 0; j<6; j++)
			{
				leafs[i].ambientLightTemp[j] = RGBExpToRGB(fleafs[i].m_AmbientLighting.m_Color[j]);
			}
		}
		delete[] fleafs;
	}
	else if(lump.version==1)
	{
		numLeafs = lump.filelen/sizeof(bspleaf_t);
		LOG("Loading %d leafs (v.%d)\n", numLeafs,lump.version);
		bspleaf_t *fleafs = new bspleaf_t[numLeafs];
		in->Seek(lump.fileofs);
		in->Read(fleafs,lump.filelen);
		leafs = new mleaf_t[numLeafs];
		memset(leafs,0,sizeof(mleaf_t)*numLeafs);
		for(int i=0; i<numLeafs; i++)
		{
			if(fleafs[i].contents<0)
			{
				LOG("leaf %d: contents %d, cluster %d, mins %d %d %d, maxs %d %d %d\n", i, fleafs[i].contents, fleafs[i].cluster, fleafs[i].mins[0],fleafs[i].mins[1],fleafs[i].mins[2], fleafs[i].maxs[0],fleafs[i].maxs[1],fleafs[i].maxs[2]);
			}
			leafs[i].contents = fleafs[i].contents;
			leafs[i].cluster = fleafs[i].cluster;
			leafs[i].area = fleafs[i].area;
			leafs[i].mins = glm::mat3(mapMtx)*glm::vec3(fleafs[i].mins[0],fleafs[i].mins[1],fleafs[i].mins[2]);
			leafs[i].maxs = glm::mat3(mapMtx)*glm::vec3(fleafs[i].maxs[0],fleafs[i].maxs[1],fleafs[i].maxs[2]);
			leafs[i].numLeafFaces = fleafs[i].numleaffaces;
			leafs[i].leafFaces = leafFaces + fleafs[i].firstleafface;
		}
		delete[] fleafs;
	}
	else
	{
		LOG("Leafs v.%d\n", lump.version);
		EngineError("Unknown leaf version");
	}
}

void map_t::LoadNodes(IFile *in, bsplump_t lump)
{
	numNodes = lump.filelen/sizeof(bspnode_t);
	LOG("Loading %d nodes\n", numNodes);
	bspnode_t *fnodes = new bspnode_t[numNodes];
	in->Seek(lump.fileofs);
	in->Read(fnodes, lump.filelen);
	nodes = new mnode_t[numNodes];
	memset(nodes,0,sizeof(mnode_t)*numNodes);
	for(int i = 0; i <numNodes; i++)
	{
		nodes[i].contents = -1;
		nodes[i].plane = planes+fnodes[i].planenum;
		nodes[i].area = fnodes[i].area;
		nodes[i].mins = glm::mat3(mapMtx)*glm::vec3(fnodes[i].mins[0],fnodes[i].mins[1],fnodes[i].mins[2]);
		nodes[i].maxs = glm::mat3(mapMtx)*glm::vec3(fnodes[i].maxs[0],fnodes[i].maxs[1],fnodes[i].maxs[2]);
		for(int n=0; n<2; n++)
		{
			if(fnodes[i].children[n]>=0)
				nodes[i].children[n] = nodes + fnodes[i].children[n];
			else
				nodes[i].children[n] = (mnode_t*)(leafs -(fnodes[i].children[n]+1));
		}
	}

	delete[] fnodes;
}

void map_t::SetupLeafsNodes()
{
	for(int i=0;i<numNodes; i++){
		nodes[i].children[0]->parent = nodes+i;
		nodes[i].children[1]->parent = nodes+i;
	}

	if(numAmbientInds){
		for(int i=0;i<numLeafs; i++){
			//TODO fix
			leafs[i].ambientLight = ambientLights+ambientInds[i].firstAmbientSample;
			for(int j = 0; j<6; j++){
				leafs[i].ambientLightTemp[j] = RGBExpToRGB(leafs[i].ambientLight->cube.m_Color[j]);
			}
		}
	}else{
		for(int i=0;i<numLeafs; i++){
			leafs[i].ambientLight = 0;
		}
	}
	//test
	/*for(int i=0;i<numLeafs; i++){
		if(!leafs[i].parent)
			LOG("Leaf %d have no parant (cluster %d, maxs %f)\n",i,leafs[i].cluster, leafs[i].maxs.x);
	}
	for(int i=0;i<numNodes; i++){
		if(!nodes[i].parent)
			LOG("Node %d have no parant\n",i);
	}*/
}

void map_t::LoadAreaPortals(IFile *in, bsplump_t alump, bsplump_t plump, bsplump_t vlump)
{
	int numAreas = alump.filelen/sizeof(bsparea_t);
	LOG("Loading areas count %d\n", numAreas);
	bsparea_t *areas = new bsparea_t[numAreas];
	in->Seek(alump.fileofs);
	in->Read(areas,alump.filelen);
	/*for(int i=0; i<numAreas; i++)
	{
		LOG("Area %d: portals first %d num %d\n", i, areas[i].firstareaportal, areas[i].numareaportals);
	}*/
	delete[] areas;

	numAreaPortals = plump.filelen/sizeof(bspareaportal_t);
	LOG("Loading area portals count %d\n", numAreaPortals);
	areaPortals = new bspareaportal_t[numAreaPortals];
	in->Seek(plump.fileofs);
	in->Read(areaPortals,plump.filelen);
	/*for(int i=0; i<numAreaPortals; i++)
	{
		LOG("Area portal %d: key %d, to %d, verts: first %d count %d\n", i, areaPortals[i].m_PortalKey, areaPortals[i].otherarea, areaPortals[i].m_FirstClipPortalVert, areaPortals[i].m_nClipPortalVerts);
	}*/
	//delete[] areaPortals;
	numAreaPortalVers = vlump.filelen/sizeof(glm::vec3);
	LOG("Loading clip portal verts count %d\n", numAreaPortalVers);
	areaPortalVerts = new glm::vec3[numAreaPortalVers];
	in->Seek(vlump.fileofs);
	in->Read(areaPortalVerts,vlump.filelen);
	/*for(int i=0; i<numAreaPortalVers; i++)
	{
		LOG("Area portal vert %d: %.1f %.1f %.1f\n", i, areaPortalVerts[i].x, areaPortalVerts[i].y,areaPortalVerts[i].z);
	}*/
}

void map_t::LoadPaklump(IFile *in, bsplump_t lump)
{
	if(!lump.filelen){
		LOG("Map without pak\n");
		return;
	}
	LOG("Loading pak %d\n", lump.filelen);

	int l = lump.filelen;
	char *d = new char[lump.filelen];
	in->Seek(lump.fileofs);
	in->Read(d,l);

	pak = zipArchive(d,l);
	g_fs.AddArchive(&pak);

#if 0
	string filePath = "maps/"+name+".zip";
	char path[256];
	if(!GetFilePath(filePath.c_str(), path, true)){
		ofstream zip_out(path, ios::binary);
		zip_out.write(pakData,lump.filelen);
		zip_out.close();
	}
#endif
}

void map_t::LoadCubemaps(IFile *in, bsplump_t lump)
{
	numCubemaps = lump.filelen/sizeof(bspcubemapsample_t);
	LOG( "Loading %d cubemaps\n", numCubemaps);
	cubemapSamples = new bspcubemapsample_t[numCubemaps];
	in->Seek(lump.fileofs);
	in->Read(cubemapSamples,lump.filelen);

	/*for(int i = 0; i<numCubemaps; i++)
	{
		LOG("Cubemap %d: pos %d %d %d, size %d\n",i,cubemapSamples[i].origin[0],cubemapSamples[i].origin[1],cubemapSamples[i].origin[2],cubemapSamples[i].size);
	}*/

	//delete[] cubemapSamples;
}

void map_t::LoadVertNormals(IFile *in, bsplump_t lumpn, bsplump_t lumpi)
{
	numVertNormals = lumpn.filelen/sizeof(Vector);
	numVertNormalIndices = lumpi.filelen/sizeof(uint16_t);
	LOG( "Loading: %d vertNormals, %d vertNormalIndices\n", numVertNormals,numVertNormalIndices);

	vertNormals = new glm::vec3[numVertNormals];
	vertNormalIndices = new uint16_t[numVertNormalIndices];
	in->Seek(lumpn.fileofs);
	in->Read(vertNormals, lumpn.filelen);
	in->Seek(lumpi.fileofs);
	in->Read(vertNormalIndices, lumpi.filelen);
}

void map_t::LoadAmbientLighting(IFile *in, bsplump_t lumpi, bsplump_t lumpl)
{
	if(!leafLumpVersion)
		return;
	if(!lumpi.filelen){
		return;
	}
	numAmbientInds = lumpi.filelen/sizeof(bspleafambientindex_t);
	numAmbientLights = lumpl.filelen/sizeof(bspleafambientlighting_t);
	LOG("Loading ambient lighting: inds %d, samples %d\n",numAmbientInds,numAmbientLights);

	if(numAmbientInds!=numLeafs)
		EngineError("numAmbientInds!=numLeafs");

	ambientInds = new bspleafambientindex_t[numAmbientInds];
	ambientLights = new bspleafambientlighting_t[numAmbientLights];
	in->Seek(lumpi.fileofs);
	in->Read(ambientInds, lumpi.filelen);
	in->Seek(lumpl.fileofs);
	in->Read(ambientLights, lumpl.filelen);
}

void map_t::LoadVis(IFile *in, bsplump_t lump)
{
	if(!lump.filelen)
	{
		LOG("Map without vis data\n");
		return;
	}
	LOG("Loading vis %d\n", lump.filelen);
	uint8_t *visData = new uint8_t[lump.filelen];
	in->Seek(lump.fileofs);
	in->Read(visData,lump.filelen);
	numVisClusters = *(int*)visData;
	LOG("num clusters %d\n",numVisClusters);
	int *clusterOfs = (int*)(visData+4);

	//TODO: pack bits
	visClustersData = new uint8_t[numVisClusters*numVisClusters];
	memset(visClustersData, 0, numVisClusters*numVisClusters);

	for(int i=0; i<numVisClusters; i++)
	{
		int ofs = clusterOfs[i*2];
		int v = 0;
		for(int j=0; j<numVisClusters; v++)
		{
			if(visData[ofs+v] == 0)
			{
				v++;
				j += 8 * visData[ofs+v];
			}
			else
			{
				for(int b = 1; b<256&&j<numVisClusters; b*=2, j++)
				{
					if(visData[ofs+v] & b)
						visClustersData[numVisClusters*i + j] = 1;
				}
			}
		}
	}

	delete[] visData;
}

void map_t::LoadDisplacements(IFile *in, bsplump_t ilump, bsplump_t vlump, CDispVert **dispVerts)
{
	if(!ilump.filelen)
		return;
	//LOG("lumps[LUMP_DISPINFO].filelen %d\nsizeof bspdispinfo_t %d\n",ilump.filelen, sizeof(bspdispinfo_t));
	numDispInfos = ilump.filelen/sizeof(bspdispinfo_t);
	LOG( "Loading %d disp infos\n",numDispInfos);
	dispInfos = new bspdispinfo_t[numDispInfos];
	in->Seek(ilump.fileofs);
	in->Read(dispInfos,ilump.filelen);

	//LOG("lumps[LUMP_DISP_VERTS].filelen %d\nsizeof CDispVert %d\n",header.lumps[LUMP_DISP_VERTS].filelen, sizeof(CDispVert));
	int numVerts = vlump.filelen/sizeof(CDispVert);
	LOG("Loading %d disp verts\n", numVerts);
	*dispVerts = new CDispVert[numVerts];
	in->Seek(vlump.fileofs);
	in->Read(*dispVerts,vlump.filelen);
}

void map_t::LoadTextures(IFile *in, bspheader_t &header)
{
	bsplump_t lp = header.lumps[LUMP_TEXDATA_STRING_TABLE];
	int numTexdataStrings = lp.filelen/sizeof(int);
	texdataStringTable = new int[numTexdataStrings];
	in->Seek(lp.fileofs);
	in->Read(texdataStringTable,lp.filelen);

	lp = header.lumps[LUMP_TEXDATA_STRING_DATA];
	LOG( "texdata string length %d\n", lp.filelen);
	texdataString = new char[lp.filelen];
	in->Seek(lp.fileofs);
	in->Read(texdataString, lp.filelen);

	lp = header.lumps[LUMP_TEXDATA];
	numTexdata = lp.filelen/sizeof(bsptexdata_t);
	LOG( "Loading %d texdata\n",numTexdata);
	texdata = new bsptexdata_t[numTexdata];
	in->Seek(lp.fileofs);
	in->Read(texdata,lp.filelen);
#if OLD_BATCHING
	for(int i = 0; i<numTexdata;i++)
	{
		char *filename = texdataString+texdataStringTable[texdata[i].nameStringTableID];
		//LOG( "texdata %d: name %s, size %dx%d\n",i,filename,texdata[i].width,texdata[i].height);

		texdata[i].nameStringTableID = GetVMTMaterial(filename);
	}
	LOG("Loaded %d materials\n", g_MaterialManager.loadedMaterials.size());
#endif
	//delete[] texdataStringTable;
	//delete[] texdataString;
	//delete[] texdata;
}

void map_t::LoadGameLump(IFile *in, bsplump_t lump)
{
	//LOG("LUMP_GAME_LUMP ofs %d len %d\n",lump.fileofs,lump.filelen);
	int lumpCount;
	in->Seek(lump.fileofs);
	in->Read(&lumpCount,4);
	//LOG("lump count %d\n",lumpCount);
	if(!lumpCount)
		return;
	bspgamelump_t *gamelumps = new bspgamelump_t[lumpCount];
	in->Read(gamelumps,sizeof(bspgamelump_t)*lumpCount);

	for(int i=0; i<lumpCount; i++)
	{
		char id[5]={0};
		*((int*)id) = gamelumps[i].id;
		id[5]=0;
		//LOG("gamelump %d id %s flags %d version %d ofs %d len %d\n",i, id,gamelumps[i].flags,gamelumps[i].version,gamelumps[i].fileofs,gamelumps[i].filelen);
		if(gamelumps[i].id==GAMELUMP_STATIC_PROPS&&load_prop)
			LoadStaticProps(in,gamelumps[i]);
	}

	delete[] gamelumps;
}

#include <gtc/matrix_transform.hpp>

void map_t::LoadStaticProps(IFile *in, bspgamelump_t lump)
{
	LOG("Static props v. %d\n",lump.version);
	//hl2 4-6, portal 5, css 6, portal2 9, csgo 10

	int dictCount;
	in->Seek(lump.fileofs);
	in->Read(&dictCount,4);
	LOG("dict count %d\n",dictCount);
	char *propNames = 0;
	if(dictCount){
		propNames = new char[dictCount*STATIC_PROP_NAME_LENGTH];
		in->Read(propNames,dictCount*STATIC_PROP_NAME_LENGTH);

		//for(int i=0;i<dictCount;i++)
		//	LOG("%d %s\n",i,propNames+i*STATIC_PROP_NAME_LENGTH);
	}

	int propLeafCount;
	in->Read(&propLeafCount,4);
	LOG("prop leaf count %d\n",propLeafCount);
	if(propLeafCount){
		//in.seekg(propLeafCount*2,ios::cur);
		propsLeafs = new uint16_t[propLeafCount];
		in->Read(propsLeafs,propLeafCount*2);
	}
	int propCount;
	in->Read(&propCount,4);
	LOG("prop count %d\n",propCount);
	if(propCount){
		int propsLen = lump.filelen-(4+dictCount*STATIC_PROP_NAME_LENGTH+4+propLeafCount*2+4);
		int propLen = propsLen/propCount;
		LOG("left %d bytes (%db per prop)\n", propsLen, propLen);
		//LOG("sizeof(StaticPropLumpBase_t) %d\n",sizeof(StaticPropLumpBase_t));
		//v5 60b
		//v9 72b
		char *staticProps = new char[propsLen];
		in->Read(staticProps, propsLen);

		vector<int> *dictProps = new vector<int>[dictCount];
		vector<glm::vec3> *propsPos = new vector<glm::vec3>[dictCount];
		StaticPropLumpBase_t *prop;
		for(int i = 0; i<propCount; i++){
			prop = (StaticPropLumpBase_t *)(staticProps+i*propLen);
			//LOG("prop %d type %d, pos %.2f %.2f %.2f, rot %.2f %.2f %.2f lightOrig %.2f %.2f %.2f\n",i,prop->m_PropType, prop->m_Origin.x, prop->m_Origin.y, prop->m_Origin.z, prop->m_Angles.x, prop->m_Angles.y, prop->m_Angles.z, prop->m_LightingOrigin.x, prop->m_LightingOrigin.y, prop->m_LightingOrigin.z);
			propsPos[prop->m_PropType].push_back(prop->m_Origin);
			dictProps[prop->m_PropType].push_back(i);
		}

		staticPropsBatches = new staticPropsBatch_t[dictCount];

		for(int i=0; i<dictCount; i++){
			staticPropsBatches[i].numInsts = propsPos[i].size();
			if(!staticPropsBatches[i].numInsts){
				staticPropsBatches[i].positions = 0;
				continue;
			}
			staticPropsBatches[i].positions = new glm::vec3[propsPos[i].size()];
			memcpy(staticPropsBatches[i].positions,&(propsPos[i][0]),propsPos[i].size()*sizeof(glm::vec3));

			const char *curPropName = propNames+i*STATIC_PROP_NAME_LENGTH;
			if(curPropName[0]=='.'&&curPropName[0]=='/'){
				curPropName += 2;
			}

			staticPropsBatches[i].model = new model_t();
			if(!staticPropsBatches[i].model->Load(curPropName, model_lod)){
				LOG("%d %d %s\n",i,propsPos[i].size(),curPropName);
				delete staticPropsBatches[i].model;
				staticPropsBatches[i].model = 0;
				staticPropsBatches[i].insts = 0;
				continue;
			}
			staticPropsBatches[i].dist = staticPropsBatches[i].model->dist*0.026*4;
			staticPropsBatches[i].insts = new staticPropInst_t[staticPropsBatches[i].numInsts];
			for(int j=0; j < staticPropsBatches[i].numInsts; j++){
				prop = (StaticPropLumpBase_t *)(staticProps+(dictProps[i][j])*propLen);
				glm::mat4 m = glm::translate(glm::mat4(1.0), prop->m_Origin);
				m = glm::rotate(m, glm::radians(prop->m_Angles.y), glm::vec3(0,0,1));
				m = glm::rotate(m, glm::radians(prop->m_Angles.x), glm::vec3(0,1,0));
				m = glm::rotate(m, glm::radians(prop->m_Angles.z), glm::vec3(1,0,0));
				staticPropsBatches[i].insts[j].modelMtx = mapMtx*m;
				staticPropsBatches[i].insts[j].leafs = leafSegment_t( propsLeafs+prop->m_FirstLeaf,prop->m_LeafCount);
				if(prop->m_Flags&STATIC_PROP_USE_LIGHTING_ORIGIN)
					staticPropsBatches[i].insts[j].lightLeaf = TraceTree(prop->m_LightingOrigin);
				else
					staticPropsBatches[i].insts[j].lightLeaf = TraceTree(prop->m_Origin);
				//LOG("prop %d dist %f %f",j,prop->m_FadeMinDist,prop->m_FadeMaxDist);
			}
		}
		delete[] staticProps;
		delete[] dictProps;
		delete[] propsPos;
	}else{
		staticPropsBatches = 0;
	}
	numStaticPropsBatches = dictCount;

	delete[] propNames;
}

void CreateLightmapTex(ColorRGBExp32 *samples, GLubyte *dest, bspface_t *face, int i, bool bump);

void map_t::CreateFaceSubmesh(Mesh *mod, mface_t *surface, ColorRGBExp32 *samples, int i)
{
	bspface_t face = faces[i];
	if(face.numedges<=0)
	{
		LOG( "Face %d have %d edges\n",i,face.numedges);
		return;
	}
	bsptexinfo_t ti = texInfos[face.texinfo];
	if((ti.flags&SURF_SKY)||(face.dispinfo!=-1))
	{
		surface->count = 0;
		return;
	}

	worldvert_t *verts = ((worldvert_t*)mod->verts)+surface->offset;
	int index = 0;
	int se = 0;
	glm::vec3 *vertex;

	int texW = texdata[ti.texdata].width;
	int texH = texdata[ti.texdata].height;
	GLuint lmx, lmy;
	int lightmapW;
	int lightmapH;
	int bh;
	if(face.lightofs != -1)
	{
		lightmapW = face.m_LightmapTextureSizeInLuxels[0]+1;
		lightmapH = face.m_LightmapTextureSizeInLuxels[1]+1;
		//cout << lightmapW << " " << lightmapH << endl;
		bh=lightmapH;
		if(ti.flags&SURF_BUMPLIGHT)
			bh*=4;

		if(!LM_AllocBlock(lightmapW, bh, &lmx, &lmy))
		{
			LM_UploadBlock();
			LM_InitBlock();

			if( !LM_AllocBlock( lightmapW, bh, &lmx, &lmy ))
			{
				LOG("AllocBlock failed on face %d",i);
#ifdef WIN32
				exit(-1);
#endif
				return;
			}
		}
	}
	for(int j=0; j < face.numedges; j++)
	{
		se = surfedges[face.firstedge+j];
		index = edges[(int)abs(se)].v[se<0];
		vertex = (glm::vec3*)(mapmesh.verts+index*3);
		verts[j].pos = *vertex;
	}
	//glm::vec3 normal;
	//normal = planes[face.planenum].normal;
	for(int j=0; j < face.numedges; j++)
	{
		verts[j].uv = glm::vec2((glm::dot(verts[j].pos,ti.textureVecsS)+ti.textureOffsS)/texW,
								(glm::dot(verts[j].pos,ti.textureVecsT)+ti.textureOffsT)/texH);
		if(face.lightofs != -1)
		{
			//verts[j*7+5] = (glm::dot(*vertex,ti.lightmapVecsS)+ti.lightmapOffsS+0.5f-face.m_LightmapTextureMinsInLuxels[0])/lightmapW;
			//verts[j*7+6] = (glm::dot(*vertex,ti.lightmapVecsT)+ti.lightmapOffsT+0.5f-face.m_LightmapTextureMinsInLuxels[1])/lightmapH;
			verts[j].lmuv = glm::vec4((glm::dot(verts[j].pos,ti.lightmapVecsS)+ti.lightmapOffsS+0.5f-face.m_LightmapTextureMinsInLuxels[0]+lmx)/BLOCK_SIZE,
									  (glm::dot(verts[j].pos,ti.lightmapVecsT)+ti.lightmapOffsT+0.5f-face.m_LightmapTextureMinsInLuxels[1]+lmy)/BLOCK_SIZE,
										0, (float)lightmapH/BLOCK_SIZE);
		}
		//verts[j].normal = normal;
		verts[j].normal = vertNormals[vertNormalIndices[surface->normalOffset+j]];
		verts[j].tangent = ti.textureVecsS;
	}

#ifndef NO_LM
	if(samples)
	{
		if(face.lightofs==-1)
		{
			//LOG("face %d have not lightmap\n",i);
#if NO_BATCHING
			lightmaps[i] = -1;
#endif
			return;
		}

		//lightmaps[i] = CreateTexture(GL_LINEAR,GL_CLAMP_TO_EDGE);
		CreateLightmapTex(samples, lightmap_buffer+(lmy*BLOCK_SIZE+lmx)*3, &face,i,ti.flags&SURF_BUMPLIGHT);
#if NO_BATCHING
		lightmaps[i] = current_lightmap_texture;
#endif
	}
#endif
}

void AdjustPoints(glm::vec3 *points, glm::vec3 startPos)
{
	int startIdx=0;
	float minDist=999999;
	int i;
	for(i=0; i<4; i++)
	{
		float dist = glm::length(startPos-points[i]);
		if(dist<minDist)
		{
			minDist=dist;
			startIdx=i;
		}
	}
	
	glm::vec3 tmpVerts[4];
	
	for(i=0; i<4; i++)
	{
		tmpVerts[i] = points[i];
	}
	
	for(i=0; i<4; i++)
	{
		points[i] = tmpVerts[(i+startIdx)%4];
	}
}

void map_t::CreateDisp(int i, ColorRGBExp32 *samples, CDispVert *dispVerts)
{
	bspface_t face = faces[dispInfos[i].m_iMapFace];
	bsptexinfo_t ti = texInfos[face.texinfo];
	char *matName = texdataString+texdataStringTable[texdata[ti.texdata].nameStringTableID];
	//LOG("disp %d mat %s\n", i, matName);
	dispBatches[i].material = g_MaterialManager.GetMaterial(matName);
	int nv = dispInfos[i].NumVerts();//4;
	dispvert_t *v = new dispvert_t[nv];
	int ni = dispInfos[i].NumTris()*3;//6;
	GLuint *inds = new GLuint[ni];
	//LOG("Disp: face %d, power %d, nv %d, ni %d, vertOffs %d\n",info->m_iMapFace,info->power,nv,ni,info->m_iDispVertStart);

	int se = 0;
	int index = 0;
	glm::vec3 *vertex;
	glm::vec3 origVerts[4];
	for(int j=0; j < 4; j++){
		se = surfedges[face.firstedge+j];
		index = edges[(int)abs(se)].v[se<0];
		vertex = (glm::vec3*)(mapmesh.verts+index*3);
		
		origVerts[j] = *vertex;
	}
	AdjustPoints(origVerts, dispInfos[i].startPosition);
	int texW = texdata[ti.texdata].width;
	int texH = texdata[ti.texdata].height;
	glm::vec2 origUV[4];
	for(int j=0; j < 4; j++){
		origUV[j] = glm::vec2((glm::dot(origVerts[j],ti.textureVecsS)+ti.textureOffsS)/texW,
			(glm::dot(origVerts[j],ti.textureVecsT)+ti.textureOffsT)/texH);
	}
	int lmW = face.m_LightmapTextureSizeInLuxels[0]+1;
	int lmH = face.m_LightmapTextureSizeInLuxels[1]+1;
	GLuint lmx, lmy;
	if(face.lightofs != -1){
		if(!LM_AllocBlock(lmW, lmH, &lmx, &lmy)){
			LM_UploadBlock();
			LM_InitBlock();

			if( !LM_AllocBlock( lmW, lmH, &lmx, &lmy )){
				LOG("AllocBlock failed on face %d",i);
#ifdef WIN32
				exit(-1);
#endif
				return;
			}
		}
		CreateLightmapTex(samples, lightmap_buffer+(lmy*BLOCK_SIZE+lmx)*3, &face, dispInfos[i].m_iMapFace,false);
		//dispLM[i]=current_lightmap_texture;
		dispBatches[i].lightmap = current_lightmap_texture;
	}
	glm::vec2 origLMUV[4];
	origLMUV[0] = glm::vec2(0.5, 0.5);
	origLMUV[1] = glm::vec2(0.5, lmH-0.5);
	origLMUV[2] = glm::vec2(lmW-0.5, lmH-0.5);
	origLMUV[3] = glm::vec2(lmW-0.5, 0.5);
	for(int j=0; j < 4; j++){
		origLMUV[j] = (origLMUV[j]+glm::vec2(lmx,lmy))/(float)BLOCK_SIZE;
	}
	
	int height = ( ( 1 << dispInfos[i].power ) + 1 );
	float invHeight=1.0/(height-1);
	
	glm::vec3 dX[2] = {
		(origVerts[1]-origVerts[0])*invHeight,
		(origVerts[2]-origVerts[3])*invHeight};
	glm::vec2 dXuv[2] = {
		(origUV[1]-origUV[0])*invHeight,
		(origUV[2]-origUV[3])*invHeight};
	glm::vec2 dXlmuv[2] = {
		(origLMUV[1]-origLMUV[0])*invHeight,
		(origLMUV[2]-origLMUV[3])*invHeight};
	
	for(int y=0; y<height; y++){
		glm::vec3 dY[2] = {
			origVerts[0]+dX[0]*(float)y,
			origVerts[3]+dX[1]*(float)y};
		glm::vec3 invSeg = (dY[1]-dY[0])*invHeight;
		glm::vec2 dYuv[2] = {
			origUV[0]+dXuv[0]*(float)y,
			origUV[3]+dXuv[1]*(float)y};
		glm::vec2 invSegUV = (dYuv[1]-dYuv[0])*invHeight;
		glm::vec2 dYlmuv[2] = {
			origLMUV[0]+dXlmuv[0]*(float)y,
			origLMUV[3]+dXlmuv[1]*(float)y};
		glm::vec2 invSegLMUV = (dYlmuv[1]-dYlmuv[0])*invHeight;
		for(int x=0; x<height; x++){
			int ndx = y * height + x;
			CDispVert *dispVert = dispVerts+(dispInfos[i].m_iDispVertStart+ndx);
			
			v[ndx].pos = dY[0]+invSeg*(float)x;
			
			v[ndx].pos += dispVert->m_vVector*dispVert->m_flDist;
			v[ndx].alpha = dispVert->m_flAlpha/255.0f;
			v[ndx].uv = dYuv[0]+invSegUV*(float)x;
			v[ndx].lmuv = dYlmuv[0]+invSegLMUV*(float)x;
		}
	}
	
	
	int cv = 0;
	GLuint *ci=inds;
	for(int y=0; y<height-1; y++){
		for(int x=0; x<height-1; x++){
			if(cv%2){
			ci[0] = cv;
			ci[1] = cv+1;
			ci[2] = cv+height;
			ci[3] = cv+1;
			ci[4] = cv+height+1;
			ci[5] = cv+height;
			}else{
			ci[0] = cv;
			ci[1] = cv+height+1;
			ci[2] = cv+height;
			ci[3] = cv;
			ci[4] = cv+1;
			ci[5] = cv+height+1;
			}
			ci+=6;
		
			cv++;
		}
		cv++;
	}
	//dispMeshes[i] = Mesh((float*)v,nv,inds,ni,GL_TRIANGLE_FAN);
	dispBatches[i].submesh = submesh_t((GLuint)inds, ni);
	dispBatches[i].verts = (float*)v;
	dispBatches[i].numVerts = nv;
}

char *ParseString(char *str, char *token, bool line=false);
glm::vec3 atovec3(const char *str);

void SetupCamera(glm::vec3 pos, glm::vec3 rot);

bool player_start_found=false;
void map_t::ParseEntities(char *data)
{
	int numEnts=0;
	LOG("Start ParseEntities\n");
	char token[256];
	std::string key;
	Entity tempEnt;
	player_start_found=false;
	while((data=ParseString(data, token))!=0)
	{
		if(token[0] != '{' )
		{
			LOG( "ParseEntities: found %s when expecting {\n", token );
			return;
		}
		tempEnt = Entity();
		while(1)
		{
			// parse key
			if((data = ParseString(data, token)) == 0)
			{
				LOG("ParseEntities: EOF without closing brace\n" );
				return;
			}
			if( token[0] == '}' )
				break; // end of desc

			key = token;
			
			// parse value	
			if((data = ParseString(data, token )) == 0) 
			{
				LOG("ParseEntities: EOF without closing brace\n" );
				return;
			}

			if( token[0] == '}' )
			{
				LOG("ParseEntities: closing brace without data\n" );
				return;
			}
			if( key== "classname" && tempEnt.classname.empty())
				tempEnt.classname = token;
			else if(key=="origin")
			{
				tempEnt.origin = atovec3(token);
			}
			else if(key=="angles")
			{
				tempEnt.angles = atovec3(token);
			}
			else if(key=="spawnflags")
			{
				tempEnt.spawnflags = atoi(token);
			}
			//else
			//	LOG("Readed: %s = %s\n",key.c_str(),token);
		}
		//LOG("classname: %s\n", tempEnt.classname.c_str());
		entities.push_back(tempEnt);
		if(tempEnt.classname=="info_player_start"&&(!player_start_found||(tempEnt.spawnflags&1)))
		{
			SetupCamera(glm::mat3(mapMtx)*(tempEnt.origin+glm::vec3(0,0,64)), tempEnt.angles);
			player_start_found=true;
		}
		numEnts++;
	}
	LOG("Done ParseEntities %d\n",numEnts);
	InitEnts();
}

void map_t::InitEnts()
{
	numEnts = entities.size()-1;//minus world_spawn
	entPos = new glm::vec3[numEnts];
	for(int i = 0; i<numEnts; i++)
	{
		entPos[i] = entities[i+1].origin;
	}
}

float pow255(float a, float p)
{
	return glm::pow(a/255.0f, p)*255.0f;
}

glm::vec3 RGBExpToRGB(ColorRGBExp32 s)
{
	float p = 1.0f/2.2f;
	float f = glm::pow(2.0f, s.exponent);
	//if(hdr) f *= 2.0f;
	return glm::pow(glm::vec3(s.r*f, s.g*f, s.b*f), glm::vec3(p));
}

void CreateLightmapTex(ColorRGBExp32 *samples, GLubyte *dest, bspface_t *face, int i, bool bump)
{
	ColorRGBExp32 *sample;
	int lightmapW = face->m_LightmapTextureSizeInLuxels[0]+1;
	int lightmapH = face->m_LightmapTextureSizeInLuxels[1]+1;
	if(bump)
		lightmapH*=4;
	if(lightmapW*lightmapH==0||lightmapW*lightmapH>20000)
	{
		LOG("face %d: lightmap size %dx%d\n", i, lightmapW, lightmapH);
		return;
	}
	//float mul = 255.0f;
	//if(hdr) mul *= 2.0f;
	float p = 1.0f/2.2f;
	sample = samples+(face->lightofs/4);
	for(int y = 0; y<lightmapH; y++)
	{
		for(int x=0; x<lightmapW; x++)
		{
#if 0
			/*sample = *(samples+(f.lightofs/4)+x);
			tempSamples[x*3  ]=glm::clamp((int)ldexp(sample.r,sample.exponent),0,255);
			tempSamples[x*3+1]=glm::clamp((int)ldexp(sample.g,sample.exponent),0,255);
			tempSamples[x*3+2]=glm::clamp((int)ldexp(sample.b,sample.exponent),0,255);*/

			sample = *(samples+(face->lightofs/4)+y*lightmapW+x);
			float f = ldexp( 1.0, sample.exponent/* - (int)(128+8)*/)/255.0f;
			/*dest[0] = glm::clamp((int)(glm::pow(sample.r * f,0.4545f)*mul),0,255);
			dest[1] = glm::clamp((int)(glm::pow(sample.g * f,0.4545f)*mul),0,255);
			dest[2] = glm::clamp((int)(glm::pow(sample.b * f,0.4545f)*mul),0,255);*/
			dest[0] = glm::clamp((int)(sample.r * f*mul),0,255);
			dest[1] = glm::clamp((int)(sample.g * f*mul),0,255);
			dest[2] = glm::clamp((int)(sample.b * f*mul),0,255);
			//*((glm::tvec3<GLubyte, glm::highp>*)dest) = glm::pow((glm::vec3)*((glm::tvec3<GLubyte, glm::highp>*)dest)/255.0f,glm::vec3(0.4545f))*255.0f;
#endif
			float f = glm::pow(2.0f, sample->exponent);
			if(hdr) f *= 2.0f;
			dest[0] = glm::clamp((int)pow255(sample->r*f, p),0,255);
			dest[1] = glm::clamp((int)pow255(sample->g*f, p),0,255);
			dest[2] = glm::clamp((int)pow255(sample->b*f, p),0,255);
			dest+=3;
			sample++;
		}
		dest+=(BLOCK_SIZE-lightmapW)*3;
	}
}

void LM_InitBlock()
{
	memset( allocated, 0, sizeof( allocated ));
	memset(lightmap_buffer, 0, BLOCK_SIZE*BLOCK_SIZE*3);
}

int LM_AllocBlock( int w, int h, GLuint *x, GLuint *y )
{
	int	i, j;
	GLuint	best, best2;

	best = BLOCK_SIZE;

	for( i = 0; i < BLOCK_SIZE - w; i++ )
	{
		best2 = 0;

		for( j = 0; j < w; j++ )
		{
			if( allocated[i+j] >= best )
				break;
			if( allocated[i+j] > best2 )
				best2 = allocated[i+j];
		}

		if( j == w )
		{
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if( best + h > BLOCK_SIZE )
		return false;

	for( i = 0; i < w; i++ )
		allocated[*x + i] = best + h;

	return true;
}

void LM_UploadBlock()
{
	Log("Upload lightmap block %d\n", current_lightmap_texture);
	
	pthread_mutex_lock(&graphicsMutex);
	g_lightmaps[current_lightmap_texture].Create(BLOCK_SIZE, BLOCK_SIZE);
	g_lightmaps[current_lightmap_texture].SetFilter(GL_LINEAR, GL_LINEAR);
	g_lightmaps[current_lightmap_texture].Upload(0, GL_RGB, lightmap_buffer);
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,BLOCK_SIZE,BLOCK_SIZE,0,GL_RGB,GL_UNSIGNED_BYTE,lightmap_buffer);
	pthread_mutex_unlock(&graphicsMutex);

	current_lightmap_texture++;
}
