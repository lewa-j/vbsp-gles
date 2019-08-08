
#include <fstream>
#include <vec3.hpp>
#include <mat4x4.hpp>
using namespace std;

#include "log.h"
#include "system/FileSystem.h"
#include "graphics/platform_gl.h"
#include "graphics/texture.h"

#include "file_format/vtf.h"
#include "renderer/progs_manager.h"
#include "renderer/vbsp_material.h"

/*
int GetVTFTexture(const char *fileName)
{
	if(g_textureManager.numLoadedTextures==MAX_CACHE_TEX)
	{
		LOG( "loadedTextures overflow!");
		return 0;
	}
	int id = g_textureManager.Find(fileName);
	if(id>=0)
		return id;
	
	id = LoadVTFTexture(fileName, modelmiplevel);
	
	g_textureManager.Add(fileName, id);
	
	return id;
}*/

#if 0
GLuint LoadVTFTexture(const char *fileName, int mip )
{
	GLuint id = 0;

	string filePath = "materials/"+string(fileName)+".vtf";
	char path[256];
	GetFilePath(filePath.c_str(), path);
	ifstream input(path,ios::binary);

	if(!input)
	{
		//LOG( "File: %s not found\n",path);
		return 0;
	}

	//LOG( "Loading texture: %s\n",fileName);

	VTFFileHeader_t header;

	input.read((char*)&header, sizeof(VTFFileHeader_t));

	//cout<<"Header"<<endl;
//	LOG( "fileType: %s\n",header.fileTypeString);
//	LOG( "version: %d.%d\n",header.version[0],header.version[1]);
//	LOG( "size: %dx%d\n", header.width, header.height);
	cout<<"flags: "<<header.flags<<endl;
	cout<<"numFrames: "<<header.numFrames<<endl;
	cout<<"startFrame: "<<header.startFrame<<endl;
	cout<<"reflectivity: "<<header.reflectivity.x<<" "<<header.reflectivity.y<<" "<<header.reflectivity.z<<" "<<endl;
	cout<<"bumpScale: "<<header.bumpScale<<endl;
//	LOG( "imageFormat: %d %s\n",header.imageFormat, ((header.imageFormat == IMAGE_FORMAT_DXT1) ? "DXT1" : ((header.imageFormat == IMAGE_FORMAT_DXT5) ? "DXT5" : "Other")));
//	LOG( "numMipLevels: %d\n",(int)header.numMipLevels);
	//LOG("lowResImage size: %dx%d\n",(int)header.lowResImageWidth, (int)header.lowResImageHeight);

	if(mip>=header.numMipLevels)
		mip = header.numMipLevels-1;

	int width = header.width>>mip;
	int height = header.height>>mip;
	if(width<4)
		width = 4;
	if(height<4)
		height = 4;

	size_t texSize=0;
	if(header.imageFormat==IMAGE_FORMAT_DXT1)		//4bpp
	{
		//texSize = header.width * header.height * 0.5f;
		texSize = ((width + 3) / 4) * ((height + 3) / 4) * 8;
	}
	else if(header.imageFormat==IMAGE_FORMAT_DXT5)	//8bpp
		//texSize = header.width * header.height;
		texSize = ((width + 3) / 4) * ((height + 3) / 4) * 16;
	else
	{
		LOG( "Unknown image format\n");
		input.close();
		return 0;
	}

//	LOG( "Tex size is: %d bytes\n",texSize);
	GLubyte *texData = new GLubyte[texSize];

	int offset = header.headerSize + ((header.lowResImageWidth+3)/4)*((header.lowResImageHeight+3)/4)*8;

	if(header.imageFormat==IMAGE_FORMAT_DXT1)
		offset += computeMipsSize(width,height,header.numMipLevels-mip,0.5f)-texSize;
	else if(header.imageFormat==IMAGE_FORMAT_DXT5)
		offset += computeMipsSize(width,height,header.numMipLevels-mip,1)-texSize;
	else
		LOG("wut?\n");

	input.seekg(offset);
	input.read((char *)texData,texSize);

	input.close();
	
	Texture tex;
//#define DXT_FAST_DECOMPRESSOR 1
#ifdef DXT_FAST_DECOMPRESSOR
	tex.Create(width/4, height/4);
#else
	tex.Create(width, height);
#endif
	tex.SetWrap(GL_REPEAT);
	tex.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	
#ifdef WIN32
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

//GL_EXT_texture_sRGB
#define VTF_SRGB 0
#if VTF_SRGB
	if(header.imageFormat==IMAGE_FORMAT_DXT1)
		glCompressedTexImage2D(tex.target, 0, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,width,height,0,texSize,texData);
	else if(header.imageFormat==IMAGE_FORMAT_DXT5)
		glCompressedTexImage2D(tex.target, 0, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,width,height,0,texSize,texData);
	else
		LOG( "wut?\n");

#else
	if(header.imageFormat==IMAGE_FORMAT_DXT1)
		tex.Upload(GL_RGB4_S3TC,texData);
	else if(header.imageFormat==IMAGE_FORMAT_DXT5)
		tex.Upload(GL_RGBA_DXT5_S3TC,texData);
	else
		LOG( "wut?\n");
#endif
#else

#ifdef DXT_FAST_DECOMPRESSOR //very rough
	//dxt5
	int newSize = texSize/8;
	int blockSize = 16;
	int colOffs = 8;
	if(header.imageFormat==IMAGE_FORMAT_DXT1)
	{
		newSize = texSize/4;
		blockSize = 8;
		colOffs = 0;
	}
	GLubyte *newData = new GLubyte[newSize];

	for(int i=0;i<newSize/2;i++)
	{
		//wtite alpha to green
		//if(header.imageFormat==IMAGE_FORMAT_DXT5)
		//{
		//	*(((GLushort*)newData)+i) = ((*(texData+i*blockSize)/4)<<5);
		//}
		//else
		*(((GLushort*)newData)+i) = *((GLushort*)(texData+i*blockSize+colOffs));
	}
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width/4,height/4,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,newData);
	//tex.Upload(...
	delete[] newData;
#else
	int newSize;
	int format;
	int blockSize;
	int colOffs;
	int stride;
	int outputStride;
	if(header.imageFormat==IMAGE_FORMAT_DXT1)
	{
		newSize = texSize*6;
		format = GL_RGB;
		blockSize = 8;
		stride = 3;
		outputStride = width*3;
		colOffs = 0;
	}
	else //dxt5
	{
		newSize = texSize*4;
		format = GL_RGBA;
		blockSize = 16;
		stride = 4;
		outputStride = width*4;
		colOffs = 8;
	}

	GLubyte *newData = new GLubyte[newSize];

	for(int i=0;i<(int)texSize/blockSize;i++)
	{
		GLushort col0 = *(GLushort*)(texData+(i*blockSize+colOffs));
		GLushort col1 = *(GLushort*)(texData+i*blockSize+colOffs+2);
		uint64_t code = *(GLuint*)(texData+i*blockSize+colOffs+4);
		int offs = i*4*stride + i/(width/4)*outputStride*3;

		GLubyte r0 = (col0>>11)&0x1F;
		GLubyte g0 = (col0>>5)&0x3F;
		GLubyte b0 = col0&0x1F;
		r0 = (r0<<3)|(r0>>2);
		g0 = (g0<<2)|(g0>>4);
		b0 = (b0<<3)|(b0>>2);
		GLubyte r1 = (col1>>11)&0x1F;
		GLubyte g1 = (col1>>5)&0x3F;
		GLubyte b1 = col1&0x1F;
		r1 = (r1<<3)|(r1>>2);
		g1 = (g1<<2)|(g1>>4);
		b1 = (b1<<3)|(b1>>2);
		GLubyte r,g,b;

		GLubyte poscode;
		for(int y=0;y<4;y++)
		{
			for(int x=0;x<4;x++)
			{
				poscode = code>>2*(4*y+x)&3;
				if(col0>col1)
				{
					switch(poscode)
					{
					case 0:
						r = r0;
						g = g0;
						b = b0;
						break;
					case 1:
						r = r1;
						g = g1;
						b = b1;
						break;
					case 2:
						r = (2*r0+r1)/3;
						g = (2*g0+g1)/3;
						b = (2*b0+b1)/3;
						break;
					case 3:
						r = (r0+2*r1)/3;
						g = (g0+2*g1)/3;
						b = (b0+2*b1)/3;
						break;
					}
				}
				else
				{
				switch(poscode)
				{
					case 0:
						r = r0;
						g = g0;
						b = b0;
						break;
					case 1:
						r = r1;
						g = g1;
						b = b1;
						break;
					case 2:
						r = (r0+r1)/2;
						g = (g0+g1)/2;
						b = (b0+b1)/2;
						break;
					case 3:
						r = g = b = 0;
						break;
				}
				}

				newData[offs+y*outputStride+x*stride] = r;
				newData[offs+y*outputStride+x*stride+1] = g;
				newData[offs+y*outputStride+x*stride+2] = b;
			}
		}

		if(header.imageFormat==IMAGE_FORMAT_DXT1)
			continue;

		GLubyte a0 = *(texData+i*blockSize);
		GLubyte a1 = *(texData+i*blockSize+1);
		code = *(uint64_t*)(texData+i*blockSize);
		GLubyte a[8];

		a[0] = a0;
		a[1] = a1;

		/*if(a0>a1)
		{
			a[2] = (6.0f*a0 + a1)/7.0f;
			a[3] = (5.0f*a0 + 2.0f*a1)/7.0f;
			a[4] = (4.0f*a0 + 3.0f*a1)/7.0f;
			a[5] = (3.0f*a0 + 4.0f*a1)/7.0f;
			a[6] = (2.0f*a0 + 5.0f*a1)/7.0f;
			a[7] = (a0 + 6.0f*a1)/7.0f;
		}
		else
		{
			a[2] = (4.0f*a0+a1)/5.0f;
			a[3] = (3.0f*a0+2.0f*a1)/5.0f;
			a[4] = (2.0f*a0+3.0f*a1)/5.0f;
			a[5] = (a0+4.0f*a1)/5.0f;
			a[6] = 0;
			a[7] = 1.0f;
		}*/
		if( a0 <= a1 )
		{
			// use 5-alpha codebook
			for( int j = 1; j < 5; ++j )
				a[1 + j] = ( ( ( 5 - j )*a0 + j*a1 )/5 );
			a[6] = 0;
			a[7] = 255;
		}
		else
		{
			// use 7-alpha codebook
			for( int j = 1; j < 7; ++j )
				a[1 + j] = ( ( ( 7 - j )*a0 + j*a1 )/7 );
		}

		for(int y=0;y<4;y++)
		{
			for(int x=0;x<4;x++)
			{
				poscode = (code >> (3*(4*y+x)+16)) & 0x07;
				newData[offs+y*outputStride+x*stride+3] = a[poscode];
			}
		}
	}

	tex.Upload(format, newData);
	delete[] newData;
#endif//DXT_FAST_DECOMPRESSOR
#endif//WIN32
	glGenerateMipmap(GL_TEXTURE_2D);
	id = tex.id;

	delete[] texData;

	return id;
}
#endif
