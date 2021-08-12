#include "stdafx.h"

#include "Mesh.h"

void Mesh::loadMeshFromFile(const char* filename)
{
	loadMeshFromFile(filename, std::unordered_map<const char*, const char*>{});
}

void Mesh::loadMeshFromFile(const char* filename, std::unordered_map<const char*, const char*> relabelling)
{
	// Open the file
	std::ifstream fs{ filename };

	if (!fs.is_open()) {
		std::string err = "Failed to open file '" + std::string(filename) + "'.";
		throw std::exception(err.c_str());
	}

	std::string buffer;

	// Skip the first line
	std::getline(fs, buffer);

	// Now read data labels and formats
	std::vector<ElementDescriptor> fileElementDescriptors;

	getline(fs, buffer);

	std::stringstream descriptorString{ buffer };

	UINT32 index;

	while (descriptorString >> buffer) {
		ElementDescriptor e;
		e.label = buffer;
		UINT64 format;
		descriptorString >> format;

		e.format = (DXGI_FORMAT)format;

		e.offset = index;


	}
}
