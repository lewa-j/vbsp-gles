
#pragma once

struct submesh_t
{
	GLuint Offset;
	GLuint Count;
	submesh_t()
	{
		Offset = 0;
		Count = 0;
	}

	submesh_t(GLuint o, GLuint n)
	{
		Offset = o;
		Count = n;
	}
};

class Mesh
{
public:
	GLfloat *verts;
	GLuint numVerts;
	GLuint *inds;//GLushort for gles
	GLuint numInds;
	GLuint mode;
	submesh_t *submeshes;
	GLuint numSubmeshes;

	Mesh(): verts(0), numVerts(0), inds(0), numInds(0), mode(GL_TRIANGLES), submeshes(0), numSubmeshes(0)
	{}
	Mesh(float *v, int nv, int m): verts(v), numVerts(nv), inds(0), numInds(0), mode(m), submeshes(0), numSubmeshes(0)
	{}
	Mesh(float *v, int nv, GLuint *i, int ni, int m): verts(v), numVerts(nv), inds(i), numInds(ni), mode(m), submeshes(0), numSubmeshes(0)
	{}

	void Draw();
};

