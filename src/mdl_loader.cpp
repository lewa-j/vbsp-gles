
#include <fstream>
//#include <assert.h>
#ifdef WIN32
	#include <cstring>
#endif
using namespace std;

#include <glm.hpp>

#include "engine.h"
#include "log.h"
#include "system/FileSystem.h"
#include "graphics/platform_gl.h"
#include "renderer/vbsp_material.h"
#include "file_format/mdl.h"
//#include "mesh.h"
#include "graphics/texture.h"
#include "vbsp.h"

const char *cur_modelLoading=0;//temp
bool model_t::Load(const char *fileName, int lod)
{
	cur_modelLoading = fileName;
	//double startTime = GetTime();
	
	//string filePath = "models/"+string(fileName)+".mdl";
	string filePath(fileName);
	IFile *mdlfile = g_fs.Open(filePath.c_str());
	if(!mdlfile){
		//LOG( "Model: %s not found\n", fileName);
		return false;
	}
	
	//LOG( "Read model: %s\n", fileName);
	
	mdlfile->Read(&mdlheader,sizeof(studiohdr_t));
	//LOG("Header: version %d name %s length %d, bodyparts num %d index 0x%x, bone num %d index 0x%x, seq num %d index %p\n", mdlheader.version, mdlheader.name, mdlheader.length, mdlheader.numbodyparts, mdlheader.bodypartindex, mdlheader.numbones,mdlheader.boneindex, mdlheader.numlocalseq, mdlheader.localseqindex);

	if(mdlheader.numbodyparts<1){
		EngineError("Model without bodyparts");
	}
	
	//bones
	mstudiobone_t *bones = new mstudiobone_t[mdlheader.numbones];
	mdlfile->Seek(mdlheader.boneindex);
	mdlfile->Read(bones,sizeof(mstudiobone_t)*mdlheader.numbones);
	/*for(int b=0; b<mdlheader.numbones; b++){
		char bonename[64];
		mdlfile->Seek(mdlheader.boneindex+sizeof(mstudiobone_t)*b+bones[b].sznameindex);
		mdlfile.get(bonename,64);
		LOG( "bone %d: name %s, parent %d, pos (%f,%f,%f), rot(%f,%f,%f)\n",b,bonename,bones[b].parent,bones[b].pos.x,bones[b].pos.y,bones[b].pos.z, bones[b].rot.x,bones[b].rot.y,bones[b].rot.z);
	}*/
	rootbonerot = bones[0].rot;
	delete[] bones;
	
	//tex
	mstudiotexture_t *textures = new mstudiotexture_t[mdlheader.numtextures];
	mdlfile->Seek(mdlheader.textureindex);
	mdlfile->Read(textures,sizeof(mstudiotexture_t)*mdlheader.numtextures);
	/*for(int t=0; t<mdlheader.numtextures;t++){
		char texname[256];
		mdlfile->Seek(mdlheader.textureindex+(sizeof(mstudiotexture_t)*t)+textures[t].sznameindex);
		mdlfile->GetString(texname,256);
		Log( "texture %d: name %s, flags %d, used %d\n",t,texname,textures[t].flags,textures[t].used);
	}*/

	int *cdtextureOfs = new int[mdlheader.numcdtextures];
	mdlfile->Seek(mdlheader.cdtextureindex);
	mdlfile->Read(cdtextureOfs,sizeof(int)*mdlheader.numcdtextures);
	std::string *cdtextures = new std::string[mdlheader.numcdtextures];
	for(int t=0; t<mdlheader.numcdtextures; t++){
		mdlfile->Seek(cdtextureOfs[t]);
		char temp[1024];
		mdlfile->GetString(temp,1024);
		cdtextures[t]=temp;
		//Log( "texture path %d: ofs %d, %s\n", t, cdtextureOfs[t], cdtextures[t].c_str());
	}
	delete[] cdtextureOfs;

	if(mdlfile->eof())
		EngineError("mdl loading eof after texture");

	//seqdesc
	mstudioseqdesc_t *seqdesc = new mstudioseqdesc_t[mdlheader.numlocalseq];
	mdlfile->Seek(mdlheader.localseqindex);
	mdlfile->Read(seqdesc,sizeof(mstudioseqdesc_t)*mdlheader.numlocalseq);
	/*for(int s=0; s<mdlheader.numlocalseq;s++)
	{
		char seqname[64];
		mdlfile->Seek(mdlheader.localseqindex+(sizeof(mstudioseqdesc_t)*s)+seqdesc[s].szlabelindex);
		mdlfile.get(seqname,64);
		LOG( "seqdesc %d: name %s, flags %d, bb min:%.1f %.1f %.1f max:%.1f %.1f %.1f\n",s,seqname,seqdesc[s].flags, seqdesc[s].bbmin.x,seqdesc[s].bbmin.y,seqdesc[s].bbmin.z, seqdesc[s].bbmax.x,seqdesc[s].bbmax.y,seqdesc[s].bbmax.z);
	}*/
	bbox = BoundingBox(seqdesc[0].bbmin,seqdesc[0].bbmax);
	delete[] seqdesc;

	//bodyparts
	if(mdlheader.numbodyparts != 1)
		Log("%s num body parts %d\n", fileName, mdlheader.numbodyparts);
	mstudiobodyparts_t *bodyparts = new mstudiobodyparts_t[mdlheader.numbodyparts];
	mdlfile->Seek(mdlheader.bodypartindex);
	mdlfile->Read(bodyparts,sizeof(mstudiobodyparts_t)*mdlheader.numbodyparts);
	
	//for(int i=0; i<mdlheader.numbodyparts; i++)
	int bp = 0;
	{
		/*char bpname[64];
		mdlfile->Seek(mdlheader.bodypartindex+(sizeof(mstudiobodyparts_t)*i)+bodyparts[i].sznameindex);
		mdlfile.get(bpname,64);
		LOG("bodypart %d name %s models: num %d index 0x%x\n",i,bpname,bodyparts[i].nummodels,bodyparts[i].modelindex);
		*/
		if(bodyparts[bp].nummodels!=1)
			Log("%s bodyparts[%d] num models %d\n", fileName, bp, bodyparts[bp].nummodels);
		mstudiomodel_t *bpmodels = new mstudiomodel_t[bodyparts[bp].nummodels];
		memset(bpmodels,0,sizeof(mstudiomodel_t));
		mdlfile->Seek(mdlheader.bodypartindex+(sizeof(mstudiobodyparts_t)*bp)+bodyparts[bp].modelindex);
		mdlfile->Read(bpmodels,sizeof(mstudiomodel_t)*bodyparts[bp].nummodels);
		//for(int m = 0;m<bodyparts[bp].nummodels;m++)
		int m = 0;
		{
			//LOG(" model %d: name %s, meshes: num %d index 0x%x, vertices: num %d index 0x%x\n",m,bpmodels[m].name,bpmodels[m].nummeshes,bpmodels[m].meshindex, bpmodels[m].numvertices, bpmodels[m].vertexindex);
			
			if(bpmodels[m].nummeshes<=0){
				Log( "  mdl model %d without meshes",bp);
			}else{
				mstudiomesh_t *mdlMeshes = new mstudiomesh_t[bpmodels[m].nummeshes];
				mdlfile->Seek(mdlheader.bodypartindex+
						(sizeof(mstudiobodyparts_t)*bp)+bodyparts[bp].modelindex+
						(sizeof(mstudiomodel_t)*m)+bpmodels[m].meshindex);
				mdlfile->Read(mdlMeshes,sizeof(mstudiomesh_t)*bpmodels[m].nummeshes);

				/*for(int msh = 0;msh<bpmodels[m].nummeshes;msh++)
				{
					LOG("  mesh %d: material %d, vertices: num %d index 0x%x\n",msh,mdlMeshes[msh].material, mdlMeshes[msh].numvertices, mdlMeshes[msh].vertexoffset);
				}*/

				if(materials == NULL){
					numMeshes = bpmodels[m].nummeshes;
					materials = new Material*[numMeshes];
					for(size_t msh = 0; msh<numMeshes; msh++){
						int t = mdlMeshes[msh].material;
						char texname[64];
						mdlfile->Seek(mdlheader.textureindex+(sizeof(mstudiotexture_t)*t)+textures[t].sznameindex);
						mdlfile->GetString(texname,64);
						materials[msh] = FindMaterial(texname, cdtextures, mdlheader.numcdtextures);
					}
				}

				//TODO replace this by small structure
				bpmodels[m].meshindex = (int)mdlMeshes;
				//delete[] mdlMeshes;
			}
		}

		bodyparts[bp].modelindex = (int)bpmodels;
		//delete[] bpmodels;
	}
	mdlheader.bodypartindex = (int)bodyparts;
	//mdlheader.textureindex = (int)textures;
	delete[] textures;
	delete[] cdtextures;
	//
	
	g_fs.Close(mdlfile);
	//delete[] bodyparts;
	
	//vvd
	//filePath = "models/"+string(fileName)+".vvd";
	int ext = filePath.find(".mdl");
	filePath = filePath.substr(0,ext)+".vvd";
	IFile *vvdfile = g_fs.Open(filePath.c_str());
	if(!vvdfile){
		//LOG( "Model: %s.vvd not found\n", fileName);
		return false;
	}
	
	vertexFileHeader_t vvdheader;
	vvdfile->Read(&vvdheader,sizeof(vertexFileHeader_t));
	//LOG( "vvd header: version %d, numLODs %d, numLODVertexes[0] %d numFixups %d\n",vvdheader.version, vvdheader.numLODs, vvdheader.numLODVertexes[0],vvdheader.numFixups);
	if(lod>=vvdheader.numLODs)
		lod = vvdheader.numLODs-1;
	
	numVerts = vvdheader.numLODVertexes[lod];
	verts = new mstudiovertex_t[numVerts];
	if(!vvdheader.numFixups){
		vvdfile->Seek(vvdheader.vertexDataStart);
		vvdfile->Read(verts,sizeof(mstudiovertex_t)*numVerts);
	}else{
		//LOG("Start fixup vertices %s\n",fileName);
		vertexFileFixup_t *fixupTable = new vertexFileFixup_t[vvdheader.numFixups];
		vvdfile->Seek(vvdheader.fixupTableStart);
		vvdfile->Read(fixupTable,sizeof(vertexFileFixup_t)*vvdheader.numFixups);
		
		int target = 0;
		for(int f=0;f<vvdheader.numFixups;f++){
			if(fixupTable[f].lod<lod)
				continue;
			//LOG( "fixup %d, lod %d, id %d, num %d\n", f, fixupTable[f].lod, fixupTable[f].sourceVertexID, fixupTable[f].numVertexes);
			
			vvdfile->Seek(vvdheader.vertexDataStart + fixupTable[f].sourceVertexID*sizeof(mstudiovertex_t));
			vvdfile->Read((verts+target),fixupTable[f].numVertexes*sizeof(mstudiovertex_t));
			target+=fixupTable[f].numVertexes;
		}
		//LOG( "read %d fixup verts\n",target);
		delete[] fixupTable;
	}
	g_fs.Close(vvdfile);
	
	/*for(int v=0;v<min(16,vvdheader.numLODVertexes[0]);v++)
	{
		LOG("vertex %d: %f %f %f\n",v,verts[v].m_vecPosition.x,verts[v].m_vecPosition.y,verts[v].m_vecPosition.z);
	}*/

	pthread_mutex_lock(&graphicsMutex);
	vbo.Create();
	vbo.Upload(numVerts*sizeof(mstudiovertex_t),verts);
	//TODO remove verts after uploading? mb need for skinning?
	pthread_mutex_unlock(&graphicsMutex);

	//vtx
	//filePath = "models/"+string(fileName)+".vtx";
	ext = filePath.find(".vvd");
	filePath = filePath.substr(0,ext)+".vtx";
	if(!g_fs.FileExists(filePath.c_str()))
		filePath = filePath.substr(0,ext)+".dx90.vtx";
	IFile *vtxfile = g_fs.Open(filePath.c_str());
	if(!vtxfile){
		//LOG( "Model: %s.vtx not found\n", fileName);
		return false;
	}

	vtxFileHeader_t vtxHeader;
	vtxfile->Read(&vtxHeader,sizeof(vtxFileHeader_t));
	//LOG( "vtx header: version %d, numLODs %d, bodyparts num %d offset 0x%x\n",vtxHeader.version, vtxHeader.numLODs, vtxHeader.numBodyParts, vtxHeader.bodyPartOffset);

	SetLOD(lod);
	vtxBodyPartHeader_t *vtxbpheaders = ReadVTXBodyparts(vtxfile, vtxHeader, lod);
	numInds = CalcIndsCount(vtxbpheaders, lod);
	//LOG("index count %d\n",numInds);
	inds = new GLuint[numInds];
	int curInd=0;
	//for(int bph=0; bph<vtxHeader.numBodyParts; bph++)
	int bph=0;
	{
		//LOG( "vtx body part header %d: numModels %d modelOffset 0x%x\n",bph,vtxbpheaders[bph].numModels,vtxbpheaders[bph].modelOffset);
		vtxModelHeader_t *vtxModels = (vtxModelHeader_t*)vtxbpheaders[bph].modelOffset;

		//for(int mh=0; mh<vtxbpheaders[bph].numModels; mh++)
		int mh=0;
		{
			//LOG( " vtx model header %d: lods num %d offset 0x%x\n",mh,vtxModels[mh].numLODs,vtxModels[mh].lodOffset);
			vtxModelLODHeader_t *vtxLODs = (vtxModelLODHeader_t*)vtxModels[mh].lodOffset;
			
			dist = 400+vtxLODs[vtxModels[mh].numLODs-1].switchPoint;
			//LOG("dist %f\n",dist);
			
			//for(int mlh=0; mlh<1 /*vtxModels[mh].numLODs*/; mlh++)
			int mlh = lod;
			{
				//LOG( "  vtx model lod header %d: Meshes num %d offset 0x%x, switchPoint %f\n",mlh,vtxLODs[mlh].numMeshes ,vtxLODs[mlh].meshOffset , vtxLODs[mlh].switchPoint);
				if(vtxLODs[mlh].numMeshes<=0){
					Log("   model %d without meshes\n",mlh);
				}else{
					vtxMeshHeader_t *vtxMeshes = (vtxMeshHeader_t*)vtxLODs[mlh].meshOffset;
					indsOfs = new submesh_t[vtxLODs[mlh].numMeshes];

					assert(vtxLODs[mlh].numMeshes<1024);
					for(int msh=0; msh<vtxLODs[mlh].numMeshes; msh++){
						//LOG( "   vtx mesh header %d: StripGroups num %d offset 0x%x, flags %d\n",msh,vtxMeshes[msh].numStripGroups,vtxMeshes[msh].stripGroupHeaderOffset,vtxMeshes[msh].flags);
						vtxStripGroupHeader_t *stripGroups = (vtxStripGroupHeader_t*)vtxMeshes[msh].stripGroupHeaderOffset;
						indsOfs[msh].Count=0;
						indsOfs[msh].Offset = curInd;
						for(int sg=0; sg<vtxMeshes[msh].numStripGroups; sg++){
							//LOG( "    vtx strip group %d: Verts num %d offset 0x%x, Indices num %d offset 0x%x, Strips num %d offset 0x%x, flags %d\n",sg,stripGroups[sg].numVerts,stripGroups[sg].vertOffset,stripGroups[sg].numIndices,stripGroups[sg].indexOffset,stripGroups[sg].numStrips,stripGroups[sg].stripOffset,stripGroups[sg].flags);
							vtxVertex_t *vtxVerts = new vtxVertex_t[stripGroups[sg].numVerts];
							vtxfile->Seek(stripGroups[sg].vertOffset);
							vtxfile->Read(vtxVerts,sizeof(vtxVertex_t)*stripGroups[sg].numVerts);

							uint16_t *vtxInds = new uint16_t[stripGroups[sg].numIndices];
							vtxfile->Seek(stripGroups[sg].indexOffset);
							vtxfile->Read(vtxInds,sizeof(uint16_t)*stripGroups[sg].numIndices);

							//inds[msh] = new GLuint[stripGroups[sg].numIndices];
							//numInds[msh] = stripGroups[sg].numIndices;
							for(int ind=0; ind<stripGroups[sg].numIndices; ind++)
							{
								//inds[v]=vtxVerts[v].origMeshVertID;
								int vertOffs = ((mstudiomesh_t*) ((mstudiomodel_t*)bodyparts[bph].modelindex)[mh].meshindex)[msh].vertexoffset;
								//inds[msh][ind] = vtxVerts[vtxInds[ind]].origMeshVertID+vertOffs;
								inds[curInd+ind] = vertOffs+vtxVerts[vtxInds[ind]].origMeshVertID;
							}
							curInd += stripGroups[sg].numIndices;
							indsOfs[msh].Count += stripGroups[sg].numIndices;
							delete[] vtxVerts;
							delete[] vtxInds;
						}
						//curInd += indsOfs[msh].numVerts;
						//LOG("ind %d ofs %d num %d\n",msh,indsOfs[msh].vertOffset,indsOfs[msh].numVerts);
					}
				}
			}
		}
	}
	ClearVTXBodyparts(vtxbpheaders, lod);

	g_fs.Close(vtxfile);

	pthread_mutex_lock(&graphicsMutex);
	ibo.Create();
	ibo.Upload(numInds*sizeof(int), inds);
	pthread_mutex_unlock(&graphicsMutex);
	
	loaded = true;
	//LOG("Model %s loaded (%f s)\n",fileName,GetTime()-startTime);
	return true;
}

Material *model_t::FindMaterial(const char *name, std::string *paths, int numPaths)
{
	for(int t = 0; t<numPaths; t++){
		string temp = paths[t]+name;
		for(size_t i=0; i<temp.size();i++){
			temp[i] = tolower(temp[i]);
			if(temp[i]=='\\')
				temp[i] = '/';
		}
		//Log("FindMaterial() %d %s\n", t, temp.c_str());
		if(g_fs.FileExists((string("materials/")+temp+".vmt").c_str()))
			return g_MaterialManager.GetMaterial(temp.c_str());
	}
	Log("model: %s, tex %s\n",cur_modelLoading,name);
	for(int t = 0; t<numPaths; t++){
		Log("texpath %d: %s\n",t,paths[t].c_str());
	}
	return g_MaterialManager.GetMaterial((paths[0]+name).c_str(),true);
}

void model_t::SetLOD(int l)
{
	int curVert = 0;
	mstudiobodyparts_t *bodyparts = (mstudiobodyparts_t*)mdlheader.bodypartindex;
	//for(int i=0; i<mdlheader.numbodyparts; i++)
	int bp = 0;
	{
		if(bodyparts[bp].nummodels!=1)
			Log("bodyparts[%d] num models %d\n", bp, bodyparts[bp].nummodels);
		mstudiomodel_t *bpmodels = (mstudiomodel_t*)bodyparts[bp].modelindex;
		for(int m = 0; m<bodyparts[bp].nummodels; m++){
			if(bpmodels[m].nummeshes<=0){
				Log( "  mdl model %d without meshes",bp);
			}else{
				mstudiomesh_t *mdlMeshes = (mstudiomesh_t*)bpmodels[m].meshindex;
				
				numMeshes = bpmodels[m].nummeshes;
				for(size_t msh = 0; msh<numMeshes; msh++){
					mdlMeshes[msh].numvertices = mdlMeshes[msh].vertexdata.numLODVertexes[l];
					mdlMeshes[msh].vertexoffset = curVert;
					curVert += mdlMeshes[msh].numvertices;
				}
			}
		}
	}
}

vtxBodyPartHeader_t *model_t::ReadVTXBodyparts(IFile *vtxFile, vtxFileHeader_t &vtxHeader, int lod)
{
	vtxBodyPartHeader_t *vtxbpheaders = new vtxBodyPartHeader_t[vtxHeader.numBodyParts];
	vtxFile->Seek(vtxHeader.bodyPartOffset);
	vtxFile->Read(vtxbpheaders,sizeof(vtxBodyPartHeader_t)*vtxHeader.numBodyParts);

	//for(int bph=0; bph<vtxHeader.numBodyParts; bph++)
	int bph=0;
	{
		vtxModelHeader_t* vtxModels = new vtxModelHeader_t[vtxbpheaders[bph].numModels];
		vtxFile->Seek(vtxHeader.bodyPartOffset+(sizeof(vtxBodyPartHeader_t)*bph)+
					  vtxbpheaders[bph].modelOffset);
		vtxFile->Read(vtxModels,sizeof(vtxModelHeader_t)*vtxbpheaders[bph].numModels);

		//for(int mh=0;mh<vtxbpheaders[bph].numModels;mh++)
		int mh=0;
		{
			vtxModelLODHeader_t *vtxLODs = new vtxModelLODHeader_t[vtxModels[mh].numLODs];
			vtxFile->Seek(vtxHeader.bodyPartOffset+(sizeof(vtxBodyPartHeader_t)*bph)+
						  vtxbpheaders[bph].modelOffset+(sizeof(vtxModelHeader_t)*mh)+
						  vtxModels[mh].lodOffset);
			vtxFile->Read(vtxLODs,sizeof(vtxModelLODHeader_t)*vtxModels[mh].numLODs);

			//for(int mlh=0;mlh<1 /*vtxModels[mh].numLODs*/;mlh++)
			int mlh = lod;
			{
				if(vtxLODs[mlh].numMeshes)
				{
					vtxMeshHeader_t *vtxMeshes = new vtxMeshHeader_t[vtxLODs[mlh].numMeshes];
					vtxFile->Seek(vtxHeader.bodyPartOffset+(sizeof(vtxBodyPartHeader_t)*bph)+
								  vtxbpheaders[bph].modelOffset+(sizeof(vtxModelHeader_t)*mh)+
								  vtxModels[mh].lodOffset+(sizeof(vtxModelLODHeader_t)*mlh)+
								  vtxLODs[mlh].meshOffset);
					vtxFile->Read(vtxMeshes,sizeof(vtxMeshHeader_t)*vtxLODs[mlh].numMeshes);

					assert(vtxLODs[mlh].numMeshes<1024);
					for(int msh=0; msh<vtxLODs[mlh].numMeshes; msh++)
					{
						vtxStripGroupHeader_t *stripGroups = new vtxStripGroupHeader_t[vtxMeshes[msh].numStripGroups];
						vtxFile->Seek(vtxHeader.bodyPartOffset+(sizeof(vtxBodyPartHeader_t)*bph)+
								  vtxbpheaders[bph].modelOffset+(sizeof(vtxModelHeader_t)*mh)+
								  vtxModels[mh].lodOffset+(sizeof(vtxModelLODHeader_t)*mlh)+
								  vtxLODs[mlh].meshOffset+(sizeof(vtxMeshHeader_t)*msh)+
								  vtxMeshes[msh].stripGroupHeaderOffset);
						vtxFile->Read(stripGroups,sizeof(vtxStripGroupHeader_t)*vtxMeshes[msh].numStripGroups);

						for(int sg=0; sg<vtxMeshes[msh].numStripGroups; sg++)
						{
							int vtxOffs = vtxHeader.bodyPartOffset+(sizeof(vtxBodyPartHeader_t)*bph)+
											vtxbpheaders[bph].modelOffset+(sizeof(vtxModelHeader_t)*mh)+
											vtxModels[mh].lodOffset+(sizeof(vtxModelLODHeader_t)*mlh)+
											vtxLODs[mlh].meshOffset+(sizeof(vtxMeshHeader_t)*msh)+
											vtxMeshes[msh].stripGroupHeaderOffset+(sizeof(vtxStripGroupHeader_t)*sg);

							stripGroups[sg].vertOffset += vtxOffs;
							stripGroups[sg].indexOffset += vtxOffs;
						}
						vtxMeshes[msh].stripGroupHeaderOffset = (int)stripGroups;
					}
					vtxLODs[mlh].meshOffset = (int)vtxMeshes;
				}
			}
			vtxModels[mh].lodOffset = (int)vtxLODs;
		}
		vtxbpheaders[bph].modelOffset = (int)vtxModels;
	}
	return vtxbpheaders;
}

int model_t::CalcIndsCount(vtxBodyPartHeader_t *vtxbpheaders, int lod)
{
	int ind=0;
	//for(int bph=0; bph<vtxHeader.numBodyParts; bph++)
	int bph=0;
	{
		vtxModelHeader_t *vtxModels = (vtxModelHeader_t*)vtxbpheaders[bph].modelOffset;
		//for(int mh=0; mh<vtxbpheaders[bph].numModels; mh++)
		int mh=0;
		{
			vtxModelLODHeader_t *vtxLODs = (vtxModelLODHeader_t*)vtxModels[mh].lodOffset;
			//for(int mlh=0; mlh<1 /*vtxModels[mh].numLODs*/; mlh++)
			int mlh = lod;
			{
				vtxMeshHeader_t *vtxMeshes = (vtxMeshHeader_t*)vtxLODs[mlh].meshOffset;
				for(int msh=0; msh<vtxLODs[mlh].numMeshes; msh++)
				{
					vtxStripGroupHeader_t *stripGroups = (vtxStripGroupHeader_t*)vtxMeshes[msh].stripGroupHeaderOffset;
					for(int sg=0; sg<vtxMeshes[msh].numStripGroups; sg++)
					{
						ind += stripGroups[sg].numIndices;
					}
				}
			}
		}
	}
	return ind;
}

void model_t::ClearVTXBodyparts(vtxBodyPartHeader_t *vtxbpheaders, int lod)
{
	//for(int bph=0; bph<vtxHeader.numBodyParts; bph++)
	int bph=0;
	{
		vtxModelHeader_t *vtxModels = (vtxModelHeader_t*)vtxbpheaders[bph].modelOffset;

		//for(int mh=0; mh<vtxbpheaders[bph].numModels; mh++)
		int mh=0;
		{
			//LOG( " vtx model header %d: lods num %d offset 0x%x\n",mh,vtxModels[mh].numLODs,vtxModels[mh].lodOffset);
			vtxModelLODHeader_t *vtxLODs = (vtxModelLODHeader_t*)vtxModels[mh].lodOffset;

			//for(int mlh=0; mlh<1 /*vtxModels[mh].numLODs*/; mlh++)
			int mlh = lod;
			{
				if(vtxLODs[mlh].numMeshes)
				{
					vtxMeshHeader_t *vtxMeshes = (vtxMeshHeader_t*)vtxLODs[mlh].meshOffset;

					for(int msh=0; msh<vtxLODs[mlh].numMeshes; msh++)
					{
						vtxStripGroupHeader_t *stripGroups = (vtxStripGroupHeader_t*)vtxMeshes[msh].stripGroupHeaderOffset;

						delete[] stripGroups;
					}
					delete[] vtxMeshes;
				}
			}
			delete[] vtxLODs;
		}
		delete[] vtxModels;
	}
	delete[] vtxbpheaders;
}
