#pragma once
#include "../nclgl/Mesh.h"
#include "../nclgl/Matrix4.h"
#include "../nclgl/Vector3.h"
#include "../nclgl/Vector4.h"

class Trees
{
public:
	Trees(Vector3 position, float size, float rotate);
	~Trees();
	void DrawTree();
	void SetPos(Vector3 point) { pos = point; }
	Vector3 GetPos() { return pos; }

	void SetRotate(float rotation) { rot = rotation; }
	float GetRotate() { return rot; }

	void SetSize(float rotation) { size = rotation; }
	Vector3 GetSize() { return Vector3(size, size, size); }

	Mesh* m;
	Shader* s;
	GLuint texture;

protected:
	float size;
	Vector3 pos;
	float rot;

};

