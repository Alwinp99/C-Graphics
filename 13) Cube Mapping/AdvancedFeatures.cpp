/*
#include "AdvancedFeatures.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

#include "Trees.h"
#include "Rain.h"

AdvancedFeatures::AdvancedFeatures(Window& parent) : OGLRenderer(parent)
{
	partShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Ground026_1K_Color.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"WoodFloor051_1K_Color.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	rainTex = SOIL_load_OGL_texture(TEXTUREDIR"Snow.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg", TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg", TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
	newTree = new Trees(Vector3(4000.0f, 150.0f, 1500.0f), 75, 90);

	PlaceTree();

	if (!earthTex || !earthBump || !cubeMap || !waterTex || !ground || !rainTex)
	{
		return;

	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(rainTex, true);

	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(1.25f, 10.0f, 1.5f), Vector4(1, 1, 1, 1), 2 * heightmapSize.x);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	reflectShader = new Shader("oldReflectVertex.glsl", "oldReflectFragment.glsl");
	skyboxShader = new Shader("oldSkyboxVertex.glsl", "oldSkyboxFragment.glsl");
	lightShader = new Shader("bumpVertex.glsl", "bumpFragment.glsl");
	partShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	monsterShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() ||
		!lightShader->LoadSuccess() || !monsterShader->LoadSuccess() ||
		!shadowShader->LoadSuccess() || !partShader->LoadSuccess())
	{
		return;
	}

	part = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	monsterMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	monsterAnim = new MeshAnimation("Role_T.anm");
	monsterMat = new MeshMaterial("Role_T.mat");

	if (!part)
	{
		return;
	}

	for (int i = 0; i < monsterMesh->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry = monsterMat->GetMaterialForLayer(i);
		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		monsterTex.emplace_back(texID);
	}
		newRoot = new SceneNode();
		newRoot->SetTransform(Matrix4::Translation(Vector3(1800.0f, 150.0f, 1800.0f)) * Matrix4::Scale(Vector3(2.0f, 2.0f, 2.0f)));
		newRoot->AddChild(new CubeRobot(part));

		rainRoot = new SceneNode();
		rainRoot->SetTransform(Matrix4::Translation(Vector3(-2000.0f, 150.0f, -2000.0f)));
		rain = new Rain(100, 100, 1000);
		rainRoot->AddChild(rain);

		glGenTextures(1, &shadowTex);
		glBindTexture(GL_TEXTURE_2D, shadowTex);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(1, &shadowFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
		glDrawBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		currentFrame = 0;
		frameTime = 0.0f;
		sceneTime = 0.0f;
		waterRotate = 0.0f;
		waterCycle = 0.0f;
		lightTime = 10.0f;
		init = true;

	}


AdvancedFeatures::~AdvancedFeatures(void){
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;
	delete monsterShader;

	delete monsterMesh;
	delete monsterAnim;
	delete monsterMat;

	delete heightMap;
	delete camera;
	delete quad;
	delete sceneShader;
	delete shadowShader;
	delete newRoot;
	delete partShader;
	delete part;

	delete newTree;
	for (auto newTree : treeList) {
		delete newTree;

	}
}

void AdvancedFeatures::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	newRoot->Update(dt);
	rainRoot->Update(dt);
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % monsterAnim->GetFrameCount();
		frameTime += 1.0f / monsterAnim->GetFrameRate();
	}
}

void AdvancedFeatures::RenderScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawShadowScene();
	DrawWater();
	DrawObjects();
	DrawRain();
	DrawHeightmap();
	DrawAnimation();
	for (auto newTree : treeList) {
		CreateTrees(newTree);
	}
}

void AdvancedFeatures::MoveCamera()
{
	camera->move = !camera->move;
}

void AdvancedFeatures::RotateCamera()
{
	camera->rotate = !camera->rotate;
}

void AdvancedFeatures::DrawShadowScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, 2048, 2048);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	BindShader(monsterShader);
	SetShaderLight(*light);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 8000, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	glUniform3fv(glGetUniformLocation(monsterShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(monsterShader->GetProgram(), "diffuseTex"), 0);
	modelMatrix = Matrix4::Translation(Vector3(2000.0f, 100.0f, 2000.0f)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(90, Vector3(0, 1, 0));
	vector < Matrix4 > frameMatrices;
	const Matrix4* invBindPose = monsterMesh->GetInverseBindPose();
	const Matrix4* frameData = monsterAnim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < monsterMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(monsterShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < monsterMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, monsterTex[i]);
		monsterMesh->DrawSubMesh(i);
	}

	for (auto i : treeList) {
		BindShader(i->s);
		SetShaderLight(*light);
		glUniform3fv(glGetUniformLocation(i->s->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
		glUniform1i(glGetUniformLocation(i->s->GetProgram(), "diffuseTex"), 0);

		modelMatrix = Matrix4::Translation(i->GetPos()) * Matrix4::Scale(i->GetSize()) * Matrix4::Rotation(i->GetRotate(), Vector3(0, 1, 0));
		UpdateShaderMatrices();

		for (int i = 0; i < newTree->m->GetSubMeshCount(); ++i) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, newTree->texture);
			newTree->m->DrawSubMesh(i);
		}
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	viewMatrix = camera->BuildViewMatrix();

}

void AdvancedFeatures::DrawAnimation()
{
	BindShader(monsterShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(monsterShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(monsterShader->GetProgram(), "diffuseTex"), 0);

	modelMatrix = Matrix4::Translation(Vector3(2000.0f, 100.0f, 2000.0f)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(90, Vector3(0, 1, 0));
	UpdateShaderMatrices();
	vector < Matrix4 > frameMatrices;
	const Matrix4* invBindPose = monsterMesh->GetInverseBindPose();
	const Matrix4* frameData = monsterAnim->GetJointData(currentFrame);
	for (unsigned int i = 0; i < monsterMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(monsterShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < monsterMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, monsterTex[i]);
		monsterMesh->DrawSubMesh(i);
	}

}

void AdvancedFeatures::PlaceTree()
{
	for (int i = 0; i < 15; i++)
	{
		Trees* newTree = new Trees(Vector3(rand() % 4000 + 50, 150, rand() % 4000 + 50), 75, 90);
		treeList.push_back(newTree);
	}
}

void AdvancedFeatures::CreateTrees(Trees* tree)
{
	BindShader(tree->s);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(tree->s->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(tree->s->GetProgram(), "diffuseTex"), 0);

	modelMatrix = Matrix4::Translation(tree->GetPos()) * Matrix4::Scale(tree->GetSize()) * Matrix4::Rotation(tree->GetRotate(), Vector3(0, 1, 0));
	UpdateShaderMatrices();

	for (int i = 0; i < tree->m->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tree->texture);
		tree->m->DrawSubMesh(i);
	}
}

void AdvancedFeatures::DrawHeightmap() {
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "shadowTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();
	heightMap->Draw();
}

void AdvancedFeatures::DrawWater() {
	BindShader(reflectShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "shadowTex"), 3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix = Matrix4::Translation(hSize * 0.5f) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(-90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	UpdateShaderMatrices();
	quad->Draw();
}

void AdvancedFeatures::DrawSkybox()
{
	glDepthMask(GL_FALSE);
	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();
	glDepthMask(GL_TRUE);

}

void AdvancedFeatures::DrawObjects()
{
	BindShader(partShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(partShader->GetProgram(), "diffuseTex"), 1);
	DrawNode(newRoot, 0);
}

void AdvancedFeatures::DrawRain()
{
	BindShader(partShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(partShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rainTex);
	DrawNode(rainRoot, 1);
}

void AdvancedFeatures::DrawNode(SceneNode* n, int tex) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetSize());
		glUniformMatrix4fv(glGetUniformLocation(partShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(partShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());
		glUniform4fv(glGetUniformLocation(partShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());
		n->Draw(*this);
	}
	for (vector < SceneNode* >::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i, tex);
	}
}
*/