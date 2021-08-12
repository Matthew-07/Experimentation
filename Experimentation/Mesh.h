#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

struct MeshException : std::exception {};
struct MeshFileException : std::exception {
	MeshFileException() : std::exception("Could not open file to load mesh.") {}
};
struct MeshLoadException : std::exception {
	MeshLoadException() : std::exception("Could not load mesh from file.") {}
};

struct UVNormVertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

class Mesh
{
public:
	//Mesh(const char* path);
	void loadFromFile(const char* path);
	void unload();
	void scheduleUpload(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	void finaliseUpload() {
		vertexBufferUploadHeap.Reset();
		indexBufferUploadHeap.Reset();
	}

	void draw(ComPtr<ID3D12GraphicsCommandList> commandList);

	static Mesh GenerateGrid(
		const UINT width, const UINT height,
		const float horizontalSpacing, const float verticalSpacing,
		XMVECTOR horizontalAxis = { 1.f, 0.f, 0.f, 0.f },
		XMVECTOR verticalAxis = { 0.f, 1.f, 0.f, 0.f });

private:
    UVNormVertex* vertices;
    UINT32* indices;
	std::size_t numberOfVertices;
	std::size_t numberOfIndices;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;
	
	ComPtr<ID3D12Resource> vertexBufferUploadHeap;
	ComPtr<ID3D12Resource> indexBufferUploadHeap;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

