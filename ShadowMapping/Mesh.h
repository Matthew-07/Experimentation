#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

// This version of the Mesh class will be ablle to work with multiple kinds of vertex by taking element descriptions
class ElementDescriptor {
public:
	std::string label;
	uint64_t offset;
	//DXGI_FORMAT format;
};

class Mesh
{
public:
	Mesh(ElementDescriptor* descriptors, UINT32 numDescriptors) {		
		m_numberOfDescriptors = numDescriptors;
		m_elementDescriptors = new ElementDescriptor[numDescriptors];

		for (UINT32 i = 0; i < numDescriptors; ++i) {
			m_elementDescriptors[i] = descriptors[i];
		}
	}

	~Mesh() {
		delete[] m_elementDescriptors;
	}

	void loadMeshFromFile(const char* filename);
	void loadMeshFromFile(const char* filename, std::unordered_map<const char*, const char*> relabelling);

private:
	void* m_vertices;
	UINT32 m_numberOfVertices;
	UINT32 m_vertexSize;

	ElementDescriptor* m_elementDescriptors;
	UINT32 m_numberOfDescriptors;

	UINT32* m_indices;
	UINT32 m_numberOfIndices;
};

