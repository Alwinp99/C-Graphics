#include "FantasyScene.h"

FantasyScene::FantasyScene(Mesh* rock, Mesh* plants, Mesh* trees, Mesh* castles, Mesh* mountains, Mesh* dragons) {
	SceneNode* rocks = new SceneNode(rock, Vector4(1, 0, 0, 1));
	rocks->SetModelScale(Vector3(10, 15, 5));
	rocks->SetTransform(Matrix4::Translation(Vector3(0, 35, 0)));
	AddChild(rocks);

	plant = new SceneNode(plants, Vector4(0, 1, 0, 1));
	plant->SetModelScale(Vector3(5, 5, 5));
	plant->SetTransform(Matrix4::Translation(Vector3(0, 30, 0)));
	AddChild(plant);

	tree = new SceneNode(trees, Vector4(0, 0, 1, 1));
	tree->SetModelScale(Vector3(3, -18, 3));
	tree->SetTransform(Matrix4::Translation(Vector3(-12, 30, -1)));
	AddChild(tree);

	castle = new SceneNode(castles, Vector4(0, 0, 1, 1));
	castle->SetModelScale(Vector3(3, -18, 3));
	castle->SetTransform(Matrix4::Translation(Vector3(12, 30, -1)));
	AddChild(castle);

	SceneNode* mountain = new SceneNode(mountains, Vector4(0, 0, 1, 1));
	mountain->SetModelScale(Vector3(3, -17.5, 3));
	mountain->SetTransform(Matrix4::Translation(Vector3(-8, 0, 0)));
	AddChild(mountain);

	SceneNode* dragon = new SceneNode(dragons, Vector4(0, 0, 1, 1));
	dragon->SetModelScale(Vector3(3, -17.5, 3));
	dragon->SetTransform(Matrix4::Translation(Vector3(8, 0, 0)));
	AddChild(dragon);

}

void FantasyScene::Update(float dt) {
	transform = transform * Matrix4::Rotation(30.0f * dt, Vector3(0, 1, 0));
	plant->SetTransform(plant->GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(0, 1, 0)));
	tree->SetTransform(tree->GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(1, 0, 0)));
	castle->SetTransform(castle ->GetTransform() * Matrix4::Rotation(30.0f * dt, Vector3(1, 0, 0)));

	SceneNode::Update(dt);
}

