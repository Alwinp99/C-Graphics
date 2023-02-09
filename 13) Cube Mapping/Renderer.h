#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Frustum.h"
#include <fstream>

class Camera;
class Mesh;
class MeshAnimation;
class MeshMaterial;
class HeightMap;
class SceneNode;
//class Environment;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer();

	void LoadAssets();
	void LoadShaders();
	void LoadSceneNodes();

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	void DrawShadowScene();
	void DrawMainScene();
	void PresentScene();
	void DrawPostProcess();
	void DrawScene();
	void ShowMap();
	void CreateMonsters();

	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();

	void GenerateScreenTexture(GLuint& into, bool depth = false);

	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);

	void Move();
	void ChangeMove();
	void MoveMethod();

	Light* light;
	Light* pointLights;
	


	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	Shader* sceneShader;
	Shader* processShader;
	Shader* shadowShader;
	Shader* texShader;
	
	
	Shader* pointlightShader;
	Shader* combineShader;
	Shader* skeletalShader;


	SceneNode* root;
	Camera* camera;
	Camera* secondCamera;
	Frustum frameFrustum;
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
	

	HeightMap* heightMap;
	Mesh* quad;
	Mesh* sphere;
	Mesh* rocks;
	Mesh* trees;
	Mesh* monsterMesh;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;

	
	GLuint bufferFBO;
	GLuint bufferColourTex;
	GLuint bufferNormalTex;
	GLuint bufferDepthTex;
	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;

	GLuint postProcessFBO;
	GLuint processFBO;
	GLuint postProcessColourTex[2];
	GLuint postProcessDepthTex;

	
	GLuint shadowTex;
	GLuint shadowFBO;
	bool shadowsPres;


	GLuint cubeMap;
	GLuint waterTex;
	GLuint forestGrass;
	GLuint forestBump;
	GLuint earthTex;
	GLuint earthBump;
	GLuint desertTex;
	GLuint desertBump;
	GLuint woodTex;
	GLuint woodBump;
	GLuint treesTex;
	GLuint treesBump;
	GLuint transparentTex;

	
	GLuint texture;
	GLuint bump;

	//Animation
	float waterRotate;
	float waterCycle;

	int currentFrame;
	float frameTime;

	bool mapDisplay = false;
	bool secondViewPoint = false;
	bool thirdViewPoint = false;
};