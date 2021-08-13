#pragma once

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

struct DirectionalLight
{
	XMFLOAT4X4 shadowTransform;
	XMFLOAT3 direction;
	XMFLOAT3 color;
	float intensity;
};

class ShadowMap
{
public:
	void addPointLight();
	void addPointLight(const PointLight& data);
	void removePointLight();

	void addDirectionalLight();
	void addDirectionalLight(const DirectionalLight& data);
	void removeDirectionalLight();

	void updatePointLight(const PointLight& data, UINT32 index);
	void updateDirectionalLight(const DirectionalLight& data, UINT32 index);

	UINT32 getDescriptorCount() {
		return m_pointLights.size() * 6 + m_directionalLights.size();
	}

	void buildResources();

	void buildDescriptors(
		UINT srvIncrementSize, UINT dsvIncrementSize,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

private:
	ID3D12Device* m_device = nullptr;

	UINT m_width = 512, m_height = 512;
	DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;

	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;

	std::vector<ComPtr<ID3D12Resource>> m_shadowCubeMaps;
	std::vector<ComPtr<ID3D12Resource>> m_shadowMaps;

	std::vector<PointLight> m_pointLights;
	std::vector<DirectionalLight> m_directionalLights;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;

	UINT m_srvIncrementSize;
	UINT m_dsvIncrementSize;

	void buildDescriptors();
};

