#pragma once

#include "Mesh.h"

class Object
{
public:


private:
	Mesh& mesh;

	static std::unordered_map<std::string, Mesh> meshes;
};

