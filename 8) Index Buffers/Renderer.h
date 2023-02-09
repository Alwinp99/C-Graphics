#pragma once
#include "../nclgl/OGLRenderer.h"

class HeightMap;
class Camera;
//class Light;
class Shader;

class Renderer : public OGLRenderer
{
public: 
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;


protected:
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();


	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;


	HeightMap* heightMap;
	Mesh* quad;

	
	Camera* camera;
	Light* light;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	float waterRotate;
	float waterCycle;
	//GLuint texture;
	//GLuint bumpmap; 
};



