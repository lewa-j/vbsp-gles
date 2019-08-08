
#include <cstring>
#include <cstdlib>
#include <fstream>
using namespace std;

#include "engine.h"
#include "log.h"
#include "system/FileSystem.h"
#include "graphics/platform_gl.h"
#include "renderer/vbsp_material.h"
#include "renderer/render_vars.h"
#include "resource/vtf.h"
#include "vbsp.h"

Material *LoadVMT(const char *fileName);

Material *MaterialManager::GetMaterial(const char *name, bool model)
{
	string temp = name;

	map<string, Material*>::iterator fm = loadedMaterials.find(string(name));
	if(fm!=loadedMaterials.end())
		return fm->second;

	//not loaded
	//Material *newMat = LoadMaterial(name);
	Material *newMat = LoadVMT(name);
	if(newMat)
	{
		//LOG("Loaded material %s\n",name);
		loadedMaterials[string(name)] = newMat;
	}
	else
	{
		//LOG("Error loading material %s\n",name);
		if(model)
		{
			newMat = loadedMaterials["*m"];
		}
		else
		{
			newMat = loadedMaterials["*"];
		}
		loadedMaterials[string(name)] = newMat;
	}
	return newMat;
}

Texture *MaterialManager::GetTexture(const char *name, int loadFlags)
{
	map<string,Texture*>::iterator ft = loadedTextures.find(string(name));
	if(ft!=loadedTextures.end())
		return ft->second;

	//not loaded
#if 0
	Texture *newTex = new Texture();
	int mip = texture_mip;
	if(loadFlags&TEXTURE_NOMIP_BIT)
		mip=0;
	if(!newTex->LoadVTF(name, mip))
	{
		delete newTex;
		newTex = 0;
	}
#else
	VTFLoader vl;
	int mip = texture_mip;
	if(loadFlags&TEXTURE_NOMIP_BIT)
		mip=0;
	pthread_mutex_lock(&graphicsMutex);
	Texture *newTex = vl.Load(name,mip);
	pthread_mutex_unlock(&graphicsMutex);
#endif
	if(newTex)
	{
		//LOG("Loaded texture %s\n", name);
		loadedTextures[string(name)] = newTex;
	}
	else
	{
		LOG("Error loading texture %s\n",name);
		return &defaultTex;
	}
	return newTex;
}

void MaterialManager::AddMaterial(const char *name, Material *mat)
{
	loadedMaterials[string(name)] = mat;
}

class vmtMaterial
{
public:
	std::string name;
	std::string shader;
	std::string baseTexture;
	std::string basetexture2;
	std::string normalmap;
	bool ssbump;
	std::string envmap;
	glm::vec3 envmapTint;
	bool translucent;
	bool alphatest;
	bool additive;
	bool nomip;
	bool nocull;

	vmtMaterial(): ssbump(0), envmapTint(1), translucent(0), alphatest(0), additive(0), nomip(0),nocull(0)
	{}
	~vmtMaterial()
	{}

	bool Load(const char *filename);
};

Material *LoadVMT(const char *fileName)
{
	//double startTime = GetTime();
	vmtMaterial tempMat;
	if(tempMat.Load(fileName)){
//		LOG("VMT exists!\n");
		eProgType prog = eProgType_NULL;
		Texture *tex = 0;
		Texture *tex2 = 0;
		//worldtwotextureblend
		//unlittwotexture
		//eyerefract
		if(tempMat.shader=="lightmappedgeneric"){
			prog = eProgType_LightmappedGeneric;
		}else if(tempMat.shader == "worldvertextransition"){
			prog = eProgType_WorldVertexTransition;
		}else if(tempMat.shader == "unlitgeneric"){
			prog = eProgType_Unlit;
		}else if(tempMat.shader == "vertexlitgeneric"){
			//TODO
			prog = eProgType_VertexlitGeneric;
		}else if(tempMat.shader == "refract"){
			prog = eProgType_Refract;
			//LOG("%s refract\n",fileName);
		}else if(tempMat.shader == "black"){
			prog = eProgType_Black;
		}else if(tempMat.shader == "water"){
			prog = eProgType_Water;
		}else{
			Log("Unknown shader %s (%s)\n",tempMat.shader.c_str(),fileName);
			//return 0;
			prog = eProgType_Unlit;
		}
		int texFlags = 0;
		if(tempMat.nomip)
			texFlags = TEXTURE_NOMIP_BIT;
		if(prog != eProgType_Refract
				&&prog != eProgType_Black
				&&prog != eProgType_Water
				&&tempMat.baseTexture.empty()){
			Log("Material %s haven't texture\n",fileName);
			return 0;
		}
		if(prog == eProgType_Refract
				&&tempMat.normalmap.empty()){
			Log("RefractMaterial %s haven't normalmap\n",fileName);
			return 0;
		}
		if(!tempMat.baseTexture.empty())
			tex = g_MaterialManager.GetTexture(tempMat.baseTexture.c_str(),texFlags);
		else
			tex = &defaultTex;
		//nm.additive = tempMat.additive;

		if(prog == eProgType_LightmappedGeneric){
			if(!tempMat.envmap.empty()){
				Texture *env = g_MaterialManager.GetTexture(tempMat.envmap.c_str(), texFlags);
				if(!tempMat.normalmap.empty()){
					Texture *bump = g_MaterialManager.GetTexture(tempMat.normalmap.c_str(), texFlags);
					return new LightmappedBumpEnvMaterial(prog, tex, bump, env, tempMat.envmapTint, tempMat.translucent,tempMat.ssbump);
				}else{
					//LOG("LightmappedEnvMaterial %s loaded\n",fileName);
					return new LightmappedEnvMaterial(prog, tex, env, tempMat.envmapTint, tempMat.translucent);
				}
			}
			if(!tempMat.normalmap.empty()){
				Texture *bump = g_MaterialManager.GetTexture(tempMat.normalmap.c_str(), texFlags);
				return new LightmappedBumpMaterial(prog, tex, bump, tempMat.translucent,tempMat.ssbump);
			}
			//LOG("lmg material %s loaded (%f s)\n",fileName,GetTime()-startTime);
			return new LightmappedGenericMaterial(prog, tex, tempMat.translucent, tempMat.alphatest, tempMat.nocull);
		}else if(prog == eProgType_WorldVertexTransition){
			tex2 = g_MaterialManager.GetTexture(tempMat.basetexture2.c_str(),texFlags);
			//LOG("WVT material %s loaded (%f s)\n",fileName,GetTime()-startTime);
			return new WorldVertexTransitionMaterial(prog, tex, tex2);
		}else if(prog == eProgType_Refract){
			tex2 = g_MaterialManager.GetTexture(tempMat.normalmap.c_str(),texFlags);
			return new RefractMaterial(prog,0,tex2);
		}
		//LOG("Material %s loaded (%f s)\n",fileName,GetTime()-startTime);
		return new Material(prog, tex, tempMat.translucent, tempMat.additive, tempMat.alphatest, tempMat.nocull);
	}
	return 0;
}

char *ParseString(char *str, char *token,bool line=false);
glm::vec3 atovec3(const char *str);

bool vmtMaterial::Load(const char *fileName)
{
	string filePath = "materials/"+string(fileName)+".vmt";
	char *text = g_fs.ReadAll(filePath.c_str());
	if(!text){
		//Log( "File: %s not found\n",path);
		return false;
	}
	name = fileName;

	//Log("loading vmt %s\n", fileName);

	//
	char token[256];
	string block;
	std::string key;
	char *data=text;

	if((data=ParseString(data, token))==0)
		EngineError("vmt: found 0 when expecting shader");
	for(size_t i=0; i<strlen(token);i++){
		token[i] = tolower(token[i]);
	}
	block = token;
	shader = token;
	//LOG("shader %s\n",shader.c_str());
	int depth=0;
	if((data=ParseString(data, token))==0)
		EngineError("Unexpected end of material file");
	if(token[0] != '{' )
	{
		Log( "vmtMaterial::Load: found %s when expecting {\n", token );
		EngineError("vmtMaterial::Load");
	}
	depth++;
	//while((data=ParseString(data, token))!=0)
	{
		while(1)
		{
			// parse key
			if((data = ParseString(data, token)) == 0)
			{
				Log("vmtMaterial::Load: EOF without closing brace\n" );
				EngineError("vmtMaterial::Load");
				return false;
			}
			if( token[0] == '}' )
			{
				depth--;
				//LOG("end of block %s depth %d\n", block.c_str(), depth);
				if(depth==0)
					break; // end of desc
				continue;
			}
			for(size_t i=0; i<strlen(token);i++)
			{
				token[i] = tolower(token[i]);
			}
			key = token;
			
			// parse value	
			if((data = ParseString(data, token,true )) == 0) 
			{
				LOG("vmtMaterial::Load(%s): EOF without closing brace\n",fileName );
				EngineError("vmtMaterial::Load");
				return false;
			}
			if( token[0] == '{' && !token[1] )
			{
				depth++;
				block = key;
				//LOG("block %s depth %d\n", block.c_str(), depth);
				continue;
			}

			if( token[0] == '}' )
			{
				LOG("vmtMaterial::Load: closing brace without data\n" );
				EngineError("vmtMaterial::Load");
				return false;
			}
			for(size_t i=0; i<strlen(token);i++)
			{
				if(token[i]=='\\')
					token[i] = '/';
			}
			if(shader=="patch")
			{
				if(key=="include")
				{
					string value(token);
					string includeName = value.substr(strlen("materials/"),value.length()-strlen("materials/.vmt"));
					//LOG("material include %s\n",includeName.c_str());
					if(!Load(includeName.c_str()))
					{
						LOG("Error loading included material %s\n", includeName.c_str());
						delete[] text;
						return false;
					}
				}
			}
			if(depth==1||block=="replace"||block=="insert"||block=="lightmappedgeneric_dx9"||(hdr&&block=="hdr"))
			{
				if(key=="$basetexture"||key=="$iris")//for EyeRefract
				{
					baseTexture = token;
				}else
				if(key=="$basetexture2")
				{
					basetexture2 = token;
				}else
				if(key=="$normalmap"||key=="$bumpmap")
				{
					normalmap = token;
				}else
				if(key=="$envmap")
				{
					envmap = token;
				}else
				if(key=="$ssbump")
				{
					ssbump = atoi(token);
				}else
				if(key=="$translucent")
				{
					translucent = atoi(token);
				}else
				if(key=="$alphatest")
				{
					alphatest = atoi(token);
					//LOG("alphatest %d (%s)\n",alphatest,fileName);
				}else
				if(key=="$additive")
				{
					additive = atoi(token);
				}else
				if(key=="$nomip")
				{
					nomip = atoi(token);
				}else
				if(key=="$nocull")
				{
					nocull = atoi(token);
				}
				else if(key=="$envmaptint")
				{
					//LOG("token %s\n",token);
					envmapTint = atovec3(token);
					//LOG("Envmap tint %.2f %.2f %.2f %s\n",envmapTint.x,envmapTint.y,envmapTint.z,fileName);
				}
			}

			//else
			//	LOG("Readed: %s = %s\n",key.c_str(),token);
		}
	}
	//
	#if 0
	string line;
	int depth=0;
	string block;
	string key;
	string value;
	while(!input.eof())
	{
		getline(input, line);

		int t = line.find_first_not_of(" \t\r");
		if(t>0)
		{
			//LOG("1)t = %d\n",t);
			line.erase(0, t);
		}
		else if(t<0)
		{
			continue;
		}
		t = line.find_last_not_of("\r");
		if(t!=(int)line.size()-1)
			line.erase(t+1, 1);

		if(line.find("//")==0)
			continue;

		if(line[0]=='{')
		{
			if(depth==0)
				shader = block;
			depth++;
			//LOG("start of block %s\n",block.c_str());
			continue;
		}

		if(line[0]=='}')
		{
			depth--;
			if(!depth)
				break;
			continue;
		}
		if(depth>1)
			continue;

		for(size_t i=0; i<line.length();i++)
		{
			line[i]=tolower(line[i]);
		}

		if(line[0]=='"')
		{
			t = line.find('"',1);
			//LOG("2)t = %d (%d)\n",t,line.size());
			if(t==(int)line.size()-1)
			{
				block = line.substr(1,t-1);
				continue;
			}
			else
			{
				if(depth==0)
					LOG("Can't remove '\"' \n");
				else if(depth==1)
				{
					key = line.substr(1,t-1);
					int t2 = line.find('"',t+1);
					if(t2!=-1)
					{
						value = line.substr(t2+1,line.find('"',t2+1)-t2-1);
					}
					else
					{
						//LOG("value of %s without \" \n",key.c_str());
						value = line.substr(t+2,line.size()-1);
					}
					//LOG("%s = %s\n",key.c_str(),value.c_str());
					if(shader=="patch")
					{
						if(key=="include")
						{
							string includeName = value.substr(strlen("materials/"),value.length()-strlen("materials/.vmt"));
							if(!Load(includeName.c_str()))
							{
								LOG("Error loading included material %s\n", includeName.c_str());
								input.close();
								return false;
							}
						}
					}
					if(key=="$basetexture"||key=="$iris")//for EyeRefract
					{
						baseTexture = value;
					}
					if(key=="$translucent")
					{
						translucent = atoi(value.c_str());
					}
					if(key=="$additive")
					{
						additive = atoi(value.c_str());
					}
					continue;
				}
			}
		}
		else
		{
			t = line.find(' ');
			//LOG("2)t = %d (%d)\n",t,line.size());
			if(t==(int)string::npos)
			{
				block = line;
				continue;
			}
			else if(depth==1)
			{
				key = line.substr(0,t);
				//value = line.substr(t+1,line.size()-t-1);
				int t2 = line.find('"',t+1);
				if(t2!=-1)
				{
					value = line.substr(t2+1,line.find('"',t2+1)-t2-1);
				}
				else
				{
					value = line.substr(t+1,line.size()-t-1);
				}
				for(size_t i=0; i<value.length();i++)
				{
					if(value[i]=='\\')
						value[i]='/';
				}
				if(shader=="patch")
				{
					if(key=="include")
					{
						string includeName = value.substr(strlen("materials/"),value.length()-strlen("materials/.vmt"));
						if(!Load(includeName.c_str()))
						{
							LOG("Error loading included material %s\n", includeName.c_str());
							input.close();
							return false;
						}
					}
				}
				//LOG("%s = %s\n",key.c_str(),value.c_str());
				if(key=="$basetexture"||key=="$iris")//for EyeRefract
				{
					baseTexture = value;
				}
				if(key=="$basetexture2")
				{
					basetexture2 = value;
				}
				if(key=="$translucent")
				{
					translucent = atoi(value.c_str());
				}
				continue;
			}
		}

		LOG("%s\n",line.c_str());
	}
	#endif

	delete[] text;
	return true;
}

#if 0
bool vmtMaterial::Load(const char *fileName)
{
	char path[256];
	string filePath = "materials/"+string(fileName)+".vmt";
	GetFilePath(filePath.c_str(), path);
	ifstream input(path);
	if(!input)
	{
		//LOG( "File: %s not found\n",path);
		return false;
	}

	name = fileName;

	//LOG("loading vmt %s\n", fileName);
	string line;
	int depth=0;
	string block;
	string key;
	string value;
	while(!input.eof())
	{
		getline(input, line);

		int t = line.find_first_not_of(" \t\r");
		if(t>0)
		{
			//LOG("1)t = %d\n",t);
			line.erase(0, t);
		}
		else if(t<0)
		{
			continue;
		}
		t = line.find_last_not_of("\r");
		if(t!=(int)line.size()-1)
			line.erase(t+1, 1);

		if(line.find("//")==0)
			continue;

		if(line[0]=='{')
		{
			if(depth==0)
				shader = block;
			depth++;
			//LOG("start of block %s\n",block.c_str());
			continue;
		}

		if(line[0]=='}')
		{
			depth--;
			if(!depth)
				break;
			continue;
		}
		if(depth>1)
			continue;

		for(size_t i=0; i<line.length();i++)
		{
			line[i]=tolower(line[i]);
		}

		if(line[0]=='"')
		{
			t = line.find('"',1);
			//LOG("2)t = %d (%d)\n",t,line.size());
			if(t==(int)line.size()-1)
			{
				block = line.substr(1,t-1);
				continue;
			}
			else
			{
				if(depth==0)
					LOG("Can't remove '\"' \n");
				else if(depth==1)
				{
					key = line.substr(1,t-1);
					int t2 = line.find('"',t+1);
					if(t2!=-1)
					{
						value = line.substr(t2+1,line.find('"',t2+1)-t2-1);
					}
					else
					{
						//LOG("value of %s without \" \n",key.c_str());
						value = line.substr(t+2,line.size()-1);
					}
					//LOG("%s = %s\n",key.c_str(),value.c_str());
					if(shader=="patch")
					{
						if(key=="include")
						{
							string includeName = value.substr(strlen("materials/"),value.length()-strlen("materials/.vmt"));
							if(!Load(includeName.c_str()))
							{
								LOG("Error loading included material %s\n", includeName.c_str());
								input.close();
								return false;
							}
						}
					}
					if(key=="$basetexture"||key=="$iris")//for EyeRefract
					{
						baseTexture = value;
					}
					if(key=="$translucent")
					{
						translucent = atoi(value.c_str());
					}
					if(key=="$additive")
					{
						additive = atoi(value.c_str());
					}
					continue;
				}
			}
		}
		else
		{
			t = line.find(' ');
			//LOG("2)t = %d (%d)\n",t,line.size());
			if(t==(int)string::npos)
			{
				block = line;
				continue;
			}
			else if(depth==1)
			{
				key = line.substr(0,t);
				//value = line.substr(t+1,line.size()-t-1);
				int t2 = line.find('"',t+1);
				if(t2!=-1)
				{
					value = line.substr(t2+1,line.find('"',t2+1)-t2-1);
				}
				else
				{
					value = line.substr(t+1,line.size()-t-1);
				}
				for(size_t i=0; i<value.length();i++)
				{
					if(value[i]=='\\')
						value[i]='/';
				}
				if(shader=="patch")
				{
					if(key=="include")
					{
						string includeName = value.substr(strlen("materials/"),value.length()-strlen("materials/.vmt"));
						if(!Load(includeName.c_str()))
						{
							LOG("Error loading included material %s\n", includeName.c_str());
							input.close();
							return false;
						}
					}
				}
				//LOG("%s = %s\n",key.c_str(),value.c_str());
				if(key=="$basetexture"||key=="$iris")//for EyeRefract
				{
					baseTexture = value;
				}
				if(key=="$basetexture2")
				{
					basetexture2 = value;
				}
				if(key=="$translucent")
				{
					translucent = atoi(value.c_str());
				}
				continue;
			}
		}

		LOG("%s\n",line.c_str());
	}

	input.close();
	return true;
}
#endif
