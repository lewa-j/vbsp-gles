
#pragma once

#include <map>
#include <queue>
#include <mat3x3.hpp>
#include "graphics/glsl_prog.h"
#include "renderer/vbsp_material.h"
#include "renderer/vbsp_mesh.h"
#include "file_format/mdl.h"

class DrawCall
{
public:
	//Mesh *mesh;
	glm::mat4 *modelMtx;
	submesh_t sm;
	glm::vec3 *light;
};

class RenderList
{
	glm::mat4 modelMtx;
public:
	std::map<glslProg*, std::map<Material*, std::queue<DrawCall> > > data;
	std::map<glslProg*, std::map<Material*, std::map<model_t*, std::queue<DrawCall> > > > modelsData;

	void SetMtx(glm::mat4 mtx);
	void Clean();
	void Add(Material *m, submesh_t sm, int flags);
	void Add(model_t *md, glm::mat4 *mtx, glm::vec3 *light, int flags);
	void Flush();
	void FlushModels();
};
