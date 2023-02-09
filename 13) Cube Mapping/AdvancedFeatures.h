/*
#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/CubeRobot.h"

class Camera;
class Mesh;
class HeightMap;
class MeshAnimation;
class MeshMaterial;
class Trees;
class Rain;
struct ParticleEffects {
	Vector2 vel, pos;
	Vector4 col;
	GLfloat time;

	ParticleEffects():
		pos(0.0f, 0.0f), vel(0.0f, 0.0f), col(1.0f, 1.0f, 1.0f, 1.0f), time(0.0f) { }


};

class AdvancedFeatures : public OGLRenderer
{
public:
	AdvancedFeatures(Window& parent);
	~AdvancedFeatures(void);

	void Start();
	void UpdateScene(float dt) override;
	void RenderScene();
	void MoveCamera();
	void RotateCamera();

protected:
	void DrawShadowScene();
	void DrawMainScene();

	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	void DrawObjects();
	void DrawRain();
	void DrawNode(SceneNode* n, int tex);
	void DrawAnimation();
	void CreateTrees(Trees* tree);
	void PlaceTree();

	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	Light* light;
	HeightMap* heightMap;
	Mesh* quad;
	Camera* camera;

	GLuint earthTex;
	GLuint earthBump;
	GLuint cubeMap;
	GLuint waterTex;
	GLuint ground;
	GLuint rainTex;

	float waterRotate;
	float waterCycle;

	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint sceneDiffuse;
	GLuint sceneBump;
	float sceneTime;

	Shader* sceneShader;
	Shader* shadowShader;

	vector <Mesh*> sceneMeshes;
	vector <Matrix4> sceneTransforms;

	Mesh* monsterMesh;
	Shader* monsterShader;
	MeshAnimation* monsterAnim;
	MeshMaterial* monsterMat;
	vector <GLuint> monsterTex;

	int currentFrame;
	float frameTime;
	float lightTime;

	vector<Trees*> treeList;
	Trees* newTree;

	SceneNode* newRoot;
	Mesh* part;
	Shader* partShader;

	SceneNode* rainRoot;
	Rain* rain;



};
*/

