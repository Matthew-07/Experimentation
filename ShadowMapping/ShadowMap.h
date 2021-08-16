#pragma once
#include "Device.h"

using namespace DirectX;
using namespace Microsoft::WRL;

struct PointLight
{
	XMFLOAT3 position = { 0.f, 0.f, 0.f }; float padding;
	XMFLOAT3 color = { 1.f, 1.f, 1.f };
	float intensity = 0.f;

	PointLight() {}
	PointLight(XMFLOAT3 position, XMFLOAT3 color, float intensity) {
		this->position = position;
		this->color = color;
		this->intensity = intensity;
	}
};

struct DirectionalLight // 32
{
	XMMATRIX shadowTransform; // 16
	XMFLOAT3 direction; // 3
	float width; // 1
	XMFLOAT3 color; // 3
	float height; // 1
	XMFLOAT3 position; // 3
	float padding; // 1
	XMFLOAT3 upDirection; // 3
	float intensity; // 1
};

class ShadowMap
{
public:
	ShadowMap() : m_viewport{ 0.f, 0.f, m_width, m_height }, m_scissorRect{ 0.f, 0.f, m_width, m_height } {}

	void init(ID3D12Device* device, UINT frameCount) { m_device = device; m_frameCount = frameCount; }

	void addPointLight();
	void addPointLight(const PointLight& data);
	void removePointLight();

	void addDirectionalLight();
	void addDirectionalLight(const DirectionalLight& data);
	void removeDirectionalLight();

	void updatePointLight(const PointLight& data, UINT32 index);
	void updateDirectionalLight(const DirectionalLight& data, UINT32 index);

	void updateConstantBuffers(UINT frameIndex);

	void setPointConstantBuffer(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootArgument, UINT frameIndex, UINT lightIndex, UINT passIndex);

	void setDirectionalConstantBuffer(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootArgument, UINT frameIndex, UINT lightIndex);

	const PointLight& getPointLight(UINT index) const{
		return m_pointLights[index];
	}

	const DirectionalLight& getDirectionalLight(UINT index) const {
		return m_directionalLights[index];
	}

	UINT getNumberOfPointLights() const {
		return m_pointLights.size();
	}

	UINT getNumberOfDirectionalLights() const {
		return m_directionalLights.size();
	}

	UINT32 getSrvDescriptorCount() {
		return m_pointLights.size() + m_directionalLights.size();
	}

	UINT32 getDsvDescriptorCount() {
		return 6 * m_pointLights.size() + m_directionalLights.size();
	}

	void buildResources();

	void buildDescriptors(
		UINT srvIncrementSize, UINT dsvIncrementSize,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

	CD3DX12_GPU_DESCRIPTOR_HANDLE SRVShadowMaps()const { return m_hGpuSrv; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE SRVShadowCubeMaps()const {
		CD3DX12_GPU_DESCRIPTOR_HANDLE handle = m_hGpuSrv;
		handle.Offset(m_shadowMaps.size(), m_srvIncrementSize);
		return handle;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE DSVShadowMaps()const { return m_hCpuDsv; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE DSVShadowCubeMaps()const {
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = m_hCpuDsv;
		handle.Offset(m_shadowMaps.size(), m_dsvIncrementSize);
		return handle;
	}

	UINT getNumberOfPointLights() { return m_pointLights.size(); }
	UINT getNumberOfDirectionalLights() { return m_directionalLights.size(); }

	XMMATRIX getLightMatrix(PointLight l, UINT index);
	XMMATRIX getLightMatrix(DirectionalLight l);

	CD3DX12_VIEWPORT* getViewport() { return &m_viewport; }
	CD3DX12_RECT* getScissorRect() { return &m_scissorRect; }

	void transitionRead(ComPtr<ID3D12GraphicsCommandList> m_commandList);
	void transitionWrite(ComPtr<ID3D12GraphicsCommandList> m_commandList);

private:
	ID3D12Device* m_device;
	UINT m_frameCount;

	//UINT m_width = 512, m_height = 512;
	UINT m_width = 1024 * 4, m_height = 1024 * 4;
	DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;
	//DXGI_FORMAT m_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;

	std::vector<ComPtr<ID3D12Resource>> m_shadowCubeMaps;
	std::vector<ComPtr<ID3D12Resource>> m_shadowMaps;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	struct ConstantBuffer {
		ComPtr<ID3D12Resource> buffer;
		UINT8* pBufferStart = nullptr;
	};

	std::vector<ConstantBuffer> m_pointLightCBs;
	std::vector<ConstantBuffer> m_directionalLightCBs;

	std::vector<PointLight> m_pointLights;
	std::vector<DirectionalLight> m_directionalLights;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;

	UINT m_srvIncrementSize;
	UINT m_dsvIncrementSize;

	void buildDescriptors();
};

