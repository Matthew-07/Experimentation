#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

struct LightSource {
	XMFLOAT4 position;
	XMFLOAT3 color;
	float intensity;
};

struct ObjectConstantBuffer {
	XMFLOAT4X4 model_matrix;
	XMFLOAT4X4 mvp_matrix;
	XMFLOAT4X4 mvp_normalMatrix;
	float k_s;
	float alpha;
	float padding[14];
};

struct SceneConstantBuffer {
	//XMFLOAT4 look_direction;
	XMFLOAT4 camera_position;
	LightSource lightSources[3]; // sizeof(LightSource) = 8 * 4 = 32
	XMFLOAT3 ambient_color;
	float ambient_intensity;
	float padding[32];
};

class ShaderException : std::exception {
public:
	ShaderException() : std::exception("Failed to load and compile shaders.") {}
};

class Pipeline
{
public:
	void init(ComPtr<ID3D12Device> device) {
		m_device = device;
		
		CreateRootSignature();
		CreatePSO();
		CreateConstantBuffers();
	}

	void setObjectConstantBuffer(const ObjectConstantBuffer &data) {
		m_objectConstantBufferData = data;
	}

	void setSceneConstantBuffer(const SceneConstantBuffer &data) {
		m_sceneConstantBufferData = data;
	}

	void updateConstantBuffers(UINT frameIndex) {
		memcpy(m_pObjectCbvDataBegin + frameIndex * sizeof(m_objectConstantBufferData), &m_objectConstantBufferData, sizeof(m_objectConstantBufferData));

		memcpy(m_pSceneCbvDataBegin + frameIndex * sizeof(m_sceneConstantBufferData), &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));

		//temp
		//memset(m_pObjectCbvDataBegin, 1, sizeof(m_objectConstantBufferData));
	}

	//void setPipelineState(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT64 m_frameIndex, ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap, ComPtr<ID3D12DescriptorHeap> m_samplerHeap);
	void setPipelineState(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT64 m_frameIndex, ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap, UINT64 m_cbvSrvDescriptorSize);

	inline ComPtr<ID3D12PipelineState> getPSO() { return m_pipelineState; }
	inline ComPtr<ID3D12RootSignature> getRootSignature() { return m_rootSignature; }

private:
	ComPtr<ID3D12Device> m_device;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	ComPtr<ID3D12Resource> m_objectConstantBuffer;
	ObjectConstantBuffer m_objectConstantBufferData;
	UINT8* m_pObjectCbvDataBegin;

	ComPtr<ID3D12Resource> m_sceneConstantBuffer;
	SceneConstantBuffer m_sceneConstantBufferData;
	UINT8* m_pSceneCbvDataBegin;

	enum PipelineRootParameters : UINT32 {
		ObjectRootCBV =0,
		SceneRootCBV,
		TextureRootSRVTable,
		RootParametersCount,
	};

	void CreateRootSignature();
	void CreatePSO();
	void CreateConstantBuffers();
};

