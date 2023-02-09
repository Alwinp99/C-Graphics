#include "Trees.h"

Trees::Trees(Vector3 position, float size, float rotate) {
	m = Mesh::LoadFromMeshFile("Tree1.3ds");
	s = new Shader("treesVert.glsl", "treesFrag.glsl");
	texture = SOIL_load_OGL_texture(TEXTUREDIR"Bark008_1K_Color", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	this->pos = position;
	this->size = size;
	this->rot = rotate;

}

Trees::~Trees() {
	delete m;
	delete s;
}
