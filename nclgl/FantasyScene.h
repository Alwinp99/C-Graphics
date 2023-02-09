#pragma once
#include "../nclgl/SceneNode.h"
class FantasyScene : public SceneNode
{
public:
	FantasyScene(Mesh* rock, Mesh* plants, Mesh* trees, Mesh* castles, Mesh* mountains, Mesh* dragons);
	~FantasyScene(void) {};
	void Update(float dt) override;

protected:
	SceneNode* castle;
	SceneNode* tree;
	SceneNode* plant;
};

