#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

enum DescriptorDataType {
	_FLOAT32,
	_FLOAT64,
	_INT8,
	_INT16,
	_INT32,
	_INT64,
	_UINT8,
	_UINT16,
	_UINT32,
	_UINT64
};

// This version of the Mesh class will be able to work with multiple kinds of vertex by taking element descriptions
class ElementDescriptor {
public:
	std::string label = "";
	uint64_t offset = 0;
	
	DescriptorDataType type = _FLOAT32;
	UINT32 size = 0;

	ElementDescriptor() {}
	ElementDescriptor(std::string label, DescriptorDataType type, UINT32 size, uint64_t offset) {
		this->label = label;
		this->offset = offset;
		this->type = type;
		this->size = size;
	}
};

class Mesh
{
public:
	Mesh(ElementDescriptor* descriptors, UINT32 numDescriptors) {		
		m_numberOfDescriptors = numDescriptors;
		m_elementDescriptors = new ElementDescriptor[numDescriptors];

		m_vertexSize = 0;

		for (UINT32 i = 0; i < numDescriptors; ++i) {
			m_elementDescriptors[i] = descriptors[i];
			m_vertexSize += descriptors[i].size * getDataSize(descriptors[i].type);
		}

		if (type_map.size() == 0) {
			initTypes();
		}
	}

	Mesh(ElementDescriptor* descriptors, UINT32 numDescriptors, UINT32 vertexSize) {
		m_numberOfDescriptors = numDescriptors;
		m_elementDescriptors = new ElementDescriptor[numDescriptors];

		m_vertexSize = vertexSize;

		for (UINT32 i = 0; i < numDescriptors; ++i) {
			m_elementDescriptors[i] = descriptors[i];
		}

		if (type_map.size() == 0) {
			initTypes();
		}
	}

	~Mesh() {
		unload();
	}

	void unload() {
		delete[] m_elementDescriptors;
		delete[] m_vertices;
		delete[] m_indices;

		m_vertexBuffer.Reset();
		m_indexBuffer.Reset();
		
		vertexBufferUploadHeap.Reset();
		indexBufferUploadHeap.Reset();
	}

	void loadMeshFromFile(const char* filename);
	void loadMeshFromFile(const char* filename, std::unordered_map<const char*, const char*> relabelling);

	void scheduleUpload(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	void finaliseUpload() {
		vertexBufferUploadHeap.Reset();
		indexBufferUploadHeap.Reset();
	}

private:
	char* m_vertices;
	UINT32 m_numberOfVertices;
	UINT32 m_vertexSize;

	ElementDescriptor* m_elementDescriptors;
	UINT32 m_numberOfDescriptors;

	UINT32* m_indices;
	UINT32 m_numberOfIndices;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;

	ComPtr<ID3D12Resource> vertexBufferUploadHeap;
	ComPtr<ID3D12Resource> indexBufferUploadHeap;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	static std::unordered_map<std::string, DescriptorDataType> type_map;

	void initTypes() {
		type_map.insert(std::make_pair("f32", _FLOAT32));
		type_map.insert(std::make_pair("f64", _FLOAT64));

		type_map.insert(std::make_pair("i8", _INT8));
		type_map.insert(std::make_pair("i16", _INT16));
		type_map.insert(std::make_pair("i32", _INT32));
		type_map.insert(std::make_pair("i64", _INT64));

		type_map.insert(std::make_pair("u8", _UINT8));
		type_map.insert(std::make_pair("u16", _UINT16));
		type_map.insert(std::make_pair("u32", _UINT32));
		type_map.insert(std::make_pair("u64", _UINT64));		
	}

	constexpr UINT32 getDataSize(const DescriptorDataType type) const{
		switch (type){
		case _FLOAT64:
		case _INT64:
		case _UINT64:
			return 8;
		case _FLOAT32:
		case _INT32:
		case _UINT32:
			return 4;
		case _INT16:
		case _UINT16:
			return 2;
		case _INT8:
		case _UINT8:
			return 1;
		}
	}
};

