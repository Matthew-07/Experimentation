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
	Mesh() {}
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

	Mesh(Mesh&& obj) {
		m_numberOfVertices = obj.m_numberOfVertices;
		m_vertices = obj.m_vertices;

		m_numberOfIndices = obj.m_numberOfIndices;
		m_indices = obj.m_indices;

		obj.m_vertices = nullptr;
		obj.m_indices = nullptr;

		m_vertexSize = obj.m_vertexSize;
	}

	void unload() {
		if (m_elementDescriptors != nullptr) { delete[] m_elementDescriptors; m_elementDescriptors = nullptr; }
		if (m_vertices != nullptr) { delete[] m_vertices; m_vertices = nullptr; }
		if (m_indices != nullptr) { delete[] m_indices; m_indices = nullptr; }

		m_vertexBuffer.Reset();
		m_indexBuffer.Reset();
		
		vertexBufferUploadHeap.Reset();
		indexBufferUploadHeap.Reset();
	}

	void loadMeshFromFile(const char* filename);
	void loadMeshFromFile(const char* filename, std::unordered_map<const char*, const char*> relabelling);

	void initAsGrid(UINT width=2, UINT height=2, float horizontalSpacing=1.f, float verticalSpacing=1.f,
		float horizontalOffset=0.f, float verticalOffset=0.f);

	void scheduleUpload(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	void finaliseUpload() {
		vertexBufferUploadHeap.Reset();
		indexBufferUploadHeap.Reset();
	}

	void draw(ComPtr<ID3D12GraphicsCommandList> m_commandList);

private:
	char* m_vertices = nullptr;
	UINT32 m_numberOfVertices;
	UINT32 m_vertexSize;

	ElementDescriptor* m_elementDescriptors = nullptr;
	UINT32 m_numberOfDescriptors;

	UINT32* m_indices = nullptr;
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
		default:
			return 0;
		}
	}
	
	void convertData(const DescriptorDataType type, double data, void* destination) {
		switch (type) {
		case _FLOAT64:
			*(double*)destination = data; break;
		case _FLOAT32:
			*(float*)destination = (float)data; break;

		case _INT64:
			*(INT64*)destination = (INT64)data; break;
		case _INT32:
			*(INT32*)destination = (INT32)data; break;
		case _INT16:
			*(INT32*)destination = (INT32)data; break;
		case _INT8:
			*(INT32*)destination = (INT32)data; break;

		case _UINT64:
			*(UINT64*)destination = (UINT64)data; break;
		case _UINT32:
			*(UINT32*)destination = (UINT32)data; break;
		case _UINT16:
			*(UINT32*)destination = (UINT32)data; break;
		case _UINT8:
			*(UINT32*)destination = (UINT32)data; break;
		}
	}
};

