#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

struct PointLight
{
	XMFLOAT3 position;
	XMFLOAT3 color;
	float intensity;

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
	

private:
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;

	UINT32 m_numberOfPointLights;
	ComPtr<ID3D12Resource> *m_shadowCubeMaps;

	UINT32 m_numberOfDirectionalLights;
	ComPtr<ID3D12Resource> *m_shadowMaps;

	std::list<PointLight> m_pointLights;
	std::list<DirectionalLight> m_directionalLights;
};

