#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include <algorithm>

#define SHADOWSIZE 2048

const int POST_PASSES = 10;
const int LIGHT_NUM = 32;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	LoadAssets();
	LoadShaders();
	LoadSceneNodes();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	pointLights = new Light[LIGHT_NUM];
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)heightmapSize.x, 150.0f, rand() % (int)heightmapSize.z));
		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 1));
		l.SetRadius((250.0f + (rand() % 250)) * heightmapSize.x * heightmapSize.x);
	} 

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

	glGenTextures(1, &postProcessDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &postProcessColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, postProcessColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &postProcessFBO);
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, postProcessDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, postProcessDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !postProcessDepthTex || !postProcessColourTex[0]) {
		return;
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	camera = new Camera(100.0f, 0.0f, heightmapSize * Vector3(0.5f, 9.0f, 0.7f));
	secondCamera = new Camera(-100.0f, 0.0f, heightmapSize * Vector3(0.4f, 40.0f, 0.8f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 10);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	currentFrame = 0;
	frameTime = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	delete quad;
	delete rocks;
	delete trees;
	delete heightMap;
	delete monsterMesh;
	delete anim;
	delete material;
	glDeleteTextures(1, &waterTex);
	glDeleteTextures(1, &forestGrass);
	glDeleteTextures(1, &forestBump);
	glDeleteTextures(1, &earthTex);
	glDeleteTextures(1, &earthBump);
	glDeleteTextures(1, &desertTex);
	glDeleteTextures(1, &desertBump);
	glDeleteTextures(1, &woodTex);
	glDeleteTextures(1, &woodBump);
	glDeleteTextures(1, &treesTex);
	glDeleteTextures(1, &treesBump);
	glDeleteTextures(6, &cubeMap);
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete shadowShader;
	delete skeletalShader;
	delete texShader;
	delete processShader;
	delete root;
	delete camera;
	delete secondCamera;
	delete light;
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &postProcessDepthTex);
	glDeleteTextures(2, postProcessColourTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::LoadAssets() {
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	rocks = Mesh::LoadFromMeshFile("Rock.msh");
	trees = Mesh::LoadFromMeshFile("tree.msh");
	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	monsterMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	anim = new MeshAnimation("Role_T.anm");
	material = new MeshMaterial("Role_T.mat");

	for (int i = 0; i < monsterMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	forestGrass = SOIL_load_OGL_texture(TEXTUREDIR"Grass002_1K_Color", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	forestBump = SOIL_load_OGL_texture(TEXTUREDIR"GrassBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Stone.jfif", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"StoneBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	desertTex = SOIL_load_OGL_texture(TEXTUREDIR"Rock029_1K_Color.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	desertBump = SOIL_load_OGL_texture(TEXTUREDIR"sandBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	woodTex = SOIL_load_OGL_texture(TEXTUREDIR"Wood067_1K_Color.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	woodBump = SOIL_load_OGL_texture(TEXTUREDIR"StoneBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treesTex = SOIL_load_OGL_texture(TEXTUREDIR"treetex.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treesBump = SOIL_load_OGL_texture(TEXTUREDIR"treenormal.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	transparentTex = SOIL_load_OGL_texture(TEXTUREDIR"Facade001_1K_Color.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!quad || !sphere || !rocks || !trees || !heightMap || !waterTex || !forestGrass || !forestBump || !earthTex || !earthBump || !desertTex || !desertBump || woodTex || !woodBump || !treesTex || !treesBump || !cubeMap) {
		return;
	}

	SetTextureRepeating(forestGrass, true);
	SetTextureRepeating(forestBump, true);
	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(desertTex, true);
	SetTextureRepeating(desertBump, true);
	SetTextureRepeating(waterTex, true);
}

void Renderer::LoadShaders() {
	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");
	texShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	processShader = new Shader("TexturedVertex.glsl", "processfrag.glsl");
	sceneShader = new Shader("PerPixelVertex.glsl", "bufferFragment.glsl");
	pointlightShader = new Shader("pointlightvert.glsl", "pointlightfrag.glsl");
	combineShader = new Shader("combinevert.glsl", "combinefrag.glsl");
	skeletalShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl"); 

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() || !shadowShader->LoadSuccess() || !sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess()/*|| !mapShader->LoadSuccess() || !processShader->LoadSuccess()*/) {
		return;
	}
}

void Renderer::LoadSceneNodes() {
	root = new SceneNode();

	for (int i = 0; i < 5; i++) {
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		s->SetTransform(Matrix4::Translation(Vector3(13000, 1700.0f + 100 * i, 5000.0f + 200.0f + 2000 * i)));
		s->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
		s->SetBoundingRadius(1000.0f);
		s->SetMesh(quad);
		s->SetTexture(transparentTex);
		root->AddChild(s);
	}

	SceneNode* rock1 = new SceneNode();
	rock1->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock1->SetTransform(Matrix4::Translation(Vector3(9000, 1300.0f, 9000)) * Matrix4::Rotation(90, Vector3(0, 1, 0)));
	rock1->SetModelScale(Vector3(5, 5, 5));
	rock1->SetBoundingRadius(8000.0f);
	rock1->SetMesh(rocks);
	rock1->SetTexture(woodTex);
	rock1->SetBump(woodBump);
	root->AddChild(rock1);

	SceneNode* rock2 = new SceneNode();
	rock2->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock2->SetTransform(Matrix4::Translation(Vector3(9500, 1200.0f, 10000)));
	rock2->SetModelScale(Vector3(5, 5, 5));
	rock2->SetBoundingRadius(8000.0f);
	rock2->SetMesh(rocks);
	rock2->SetTexture(woodTex);
	rock2->SetBump(woodBump);
	root->AddChild(rock2);

	SceneNode* rock3 = new SceneNode();
	rock3->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock3->SetTransform(Matrix4::Translation(Vector3(9000, 1000.0f, 9400)));
	rock3->SetModelScale(Vector3(10, 10, 10));
	rock3->SetBoundingRadius(8000.0f);
	rock3->SetMesh(rocks);
	rock3->SetTexture(woodTex);
	rock3->SetBump(woodBump); 
	root->AddChild(rock3);
	 
	SceneNode* rock4 = new SceneNode();
	rock4->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock4->SetTransform(Matrix4::Translation(Vector3(9000, 1000.0f, 9100)) * Matrix4::Rotation(-90, Vector3(0, 1, 0)));
	rock4->SetModelScale(Vector3(10, 10, 10));
	rock4->SetBoundingRadius(8000.0f);
	rock4->SetMesh(rocks);
	rock4->SetTexture(woodTex);
	rock4->SetBump(woodBump);
	root->AddChild(rock4);

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 8; j++) {
			SceneNode* newTree = new SceneNode();
			newTree->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			newTree->SetTransform(Matrix4::Translation(Vector3(5000 + i * 200, 900.0f, 8000 + j * 100)));
			newTree->SetModelScale(Vector3(1, 1, 1));
			newTree->SetBoundingRadius(9000.0f);
			newTree->SetMesh(trees);
			newTree->SetTexture(treesTex);
			newTree->SetBump(treesBump);
			root->AddChild(newTree);
		}
	}
}

void Renderer::UpdateScene(float dt) {
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
		Move();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
		ChangeMove();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
		MoveMethod();
	}
	camera->UpdateCamera(dt);
	secondCamera->SetPosition(Vector3(camera->GetPosition().x, heightMap->GetHeightmapSize().y * 20.0f, camera->GetPosition().z));
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
	root->Update(dt);
}

void Renderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawWater();
	if (!thirdViewPoint) {
		DrawShadowScene();
		DrawMainScene();

	}
	else {
		FillBuffers();
		DrawPointLights();
		CombineBuffers();
	}

	CreateMonsters();


	if (mapDisplay) {
		if (!secondViewPoint) {
			ShowMap();
		}
		else {
			PresentScene();
			DrawPostProcess();
			DrawScene();
		}
	}
	ClearNodeLists();
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(sceneShader);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "doHeightMap"), 1);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, forestGrass);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, forestBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex2"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex2"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex3"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, desertTex);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex3"), 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, desertBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(700.0f, 15000.0f, (float)width / (float)height, 50.0f);
	modelMatrix = Matrix4::Scale(Vector3(1, 20, 1));

	UpdateShaderMatrices();

	heightMap->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() {
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	quad->Draw();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);
	viewMatrix = camera->BuildViewMatrix();
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
	if (shadowsPres == false) {
		glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
		

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, forestGrass);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, forestBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex2"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, earthTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex2"), 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, earthBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex3"), 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, desertTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex3"), 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, desertBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "shadowTex"), 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowTex);

		textureMatrix.ToIdentity();
	}

	modelMatrix = Matrix4::Scale(Vector3(1, 20, 1));
	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);
	//glDisable(GL_CULL_FACE);
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix = Matrix4::Translation(hSize * 0.5f * Vector3(1, 6, 1)) * Matrix4::Scale(hSize * 0.5f * Vector3(1, 1, 1)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 50.0f);
	UpdateShaderMatrices();

	quad->Draw();
	glEnable(GL_CULL_FACE);
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, 1, 45.0f);
	shadowMatrix = projMatrix * viewMatrix;

	shadowsPres = true;
	DrawHeightmap();
	DrawNodes();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawMainScene() {
	BindShader(lightShader);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 20000.0f, (float)width / (float)height, 45.0f);
	SetShaderLight(*light);

	shadowsPres = false;
	DrawHeightmap();
	DrawNodes();
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	BindShader(texShader);
	viewMatrix = secondCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 250000.0f, 1, 50.0f);
	modelMatrix = Matrix4::Scale(Vector3(1, 15, 1));
	UpdateShaderMatrices();

	glUniform3fv(glGetUniformLocation(texShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(texShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, forestGrass);

	glUniform1i(glGetUniformLocation(texShader->GetProgram(), "diffuseTex2"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(texShader->GetProgram(), "diffuseTex3"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, desertTex);

	heightMap->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);

	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, postProcessColourTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, postProcessColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawScene() {
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glViewport(width - 250, height - 250, 300, 200);
	BindShader(texShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, postProcessColourTex[0]);
	glUniform1i(glGetUniformLocation(texShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::ShowMap() {
	glDisable(GL_DEPTH_TEST);
	glViewport(width - 250, height - 250, 300, 200);
	BindShader(lightShader);
	projMatrix = Matrix4::Perspective(2000.0f, 9000.0f, 1, 50.0f);
	SetShaderLight(*light);
	viewMatrix = secondCamera->BuildViewMatrix();
	DrawHeightmap();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::CreateMonsters() {
	BindShader(skeletalShader);
	glUniform1i(glGetUniformLocation(skeletalShader->GetProgram(), "diffuseTex"), 0);
	modelMatrix = Matrix4::Translation(Vector3(9000, 1200.0f, 10000)) * Matrix4::Scale(Vector3(100, 100, 100));
	UpdateShaderMatrices();

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = monsterMesh->GetInverseBindPose();
	const Matrix4* frameData = anim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < monsterMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(skeletalShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < monsterMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, matTextures[i]);
		monsterMesh->DrawSubMesh(i);
	}
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) {
		BuildNodeLists(*i);
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		glDisable(GL_CULL_FACE);
		DrawNode(i);
		glEnable(GL_CULL_FACE);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

		if (shadowsPres == false) {
			glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
			//glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "doHeightMap"), 0);

			texture = n->GetTexture();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

			bump = n->GetBump();
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, bump);

			glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "shadowTex"), 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, shadowTex);
		}
		UpdateShaderMatrices();
		n->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::Move() {
	mapDisplay = !mapDisplay;
}

void Renderer::ChangeMove() {
	secondViewPoint = !secondViewPoint;
}

void Renderer::MoveMethod() {
	thirdViewPoint = !thirdViewPoint;
}