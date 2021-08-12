#pragma once

#include "Mesh.h"

using namespace DirectX;

class Object
{
public:
	

private:
	Mesh& mesh;

	static std::unordered_map<std::string, Mesh> meshes;

	XMVECTOR position;
	XMVECTOR scaling;
	XMVECTOR quaternion;
};

