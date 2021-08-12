#include "stdafx.h"

#include "Mesh.h"

int main() {
	ElementDescriptor descriptors[] =
	{
		{"position", _FLOAT32, 3, 0}
	};

	Mesh monkey{
		descriptors, 1
	};

	monkey.loadMeshFromFile("monkey.mesh");

	std::cout << "Loaded monkey.mesh successfully";
}