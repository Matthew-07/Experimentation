#include "stdafx.h"
#include "ShadowMap.h"

void ShadowMap::buildDescriptors(UINT srvIncrementSize, UINT dsvIncrementSize,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;
	m_hCpuDsv = hCpuDsv;

	buildResources();
	buildDescriptors();
}

void ShadowMap::buildResources()
{
	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	m_pointLights.clear();
	m_directionalLights.clear();

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_width;
	texDesc.Height = m_height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	for (UINT i = 0; i < m_pointLights.size() * 6; ++i) {
		ComPtr<ID3D12Resource> shadowMap;
		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&shadowMap)));

		m_shadowCubeMaps.push_back(shadowMap);
	}

	for (UINT i = 0; i < m_directionalLights.size(); ++i) {
		ComPtr<ID3D12Resource> shadowMap;
		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&shadowMap)));

		m_shadowMaps.push_back(shadowMap);
	}
}

void ShadowMap::buildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	for (auto shadowMap : m_shadowMaps) {
		// Create SRV to resource so we can sample the shadow map in a shader program.
		m_device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, m_hCpuSrv);
		m_hCpuSrv.Offset(m_srvIncrementSize);

		// Create DSV to resource so we can render to the shadow map.
		m_device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, m_hCpuDsv);
		m_hCpuDsv.Offset(m_dsvIncrementSize);
	}
}
