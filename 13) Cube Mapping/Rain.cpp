/*
#include "Rain.h"

Rain::Rain(float x, float y, float num) {
	this->type = x;
	this->b = y;
	this->numRain = num;

	mesh = Mesh::LoadFromMeshFile("Sphere.msh");
	tex = SOIL_load_OGL_texture(TEXTUREDIR"Ice003_1K_Color.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	for (int i = 0; i < numRain; i++) {
		ParticleEffects part;
		part.isRaining = true;
		part.pos = Vector3(rand() % 16384, 000, rand() % 16384);
		float fallSpeed = (rand() % 15) + 8;
		part.fallSpeed = Vector3(0, -fallSpeed, -3);
		particles.push_back(part);

		SceneNode* rain = new SceneNode();
		rain->SetMesh(this->mesh);
		rain->SetTransform(Matrix4::Translation(part.pos));
		rain->SetSize(Vector3(10, 10, 10));
		AddChild(rain);
		
	 }
  }

void Rain::Update(float dt) {
	
	for (int i = 0; i < numRain; i++) {
		if (children[i]->GetTransform().GetPositionVector().y <= 0) {
			children[i]->SetTransform(Matrix4::Translation(Vector3(rand() % 16384, 8000, rand() % 16384)));
			float fallSpeed = (rand() % 15) + 8;
			particles[i].fallSpeed = Vector3(0, -fallSpeed, 0);
		}

		children[i]->SetTransform(children[i]->GetTransform() * Matrix4::Translation(particles[i].fallSpeed));
	}

	SceneNode::Update(dt);
	
}
*/


