#include "stdafx.h"
#include "ShadowMap.h"

#include "Application.h"

void ShadowMap::buildDescriptors(UINT srvIncrementSize, UINT dsvIncrementSize,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;
	m_hCpuDsv = hCpuDsv;

	m_srvIncrementSize = srvIncrementSize;
	m_dsvIncrementSize = dsvIncrementSize;

	buildResources();
	buildDescriptors();
}

XMMATRIX ShadowMap::getLightMatrix(PointLight l, UINT index)
{
	XMVECTOR direction;
	XMVECTOR upDir;
	switch (index) {
	case 0:
		direction = XMVectorSet( 1.f, 0.f, 0.f, 0.f );
		upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f); break;
	case 1:
		direction = XMVectorSet(-1.f, 0.f, 0.f, 0.f );
		upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f); break;

	case 2:
		direction = XMVectorSet(0.f, 1.f, 0.f, 0.f );
		upDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	case 3:
		direction = XMVectorSet(0.f, -1.f, 0.f, 0.f );
		upDir = XMVectorSet(0.f, 0.f, 1.f, 0.f); break;

	case 4:
		direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f); break;
	default:
		direction = XMVectorSet(0.f, 0.f, -1.f, 0.f );
		upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f); break;
	}

	XMVECTOR position = XMLoadFloat3(&l.position);
	return XMMatrixLookToLH(position, direction, upDir) *
		XMMatrixPerspectiveFovLH(XMConvertToRadians(90.f), 1.f, 0.1f, 100.f);
}

XMMATRIX ShadowMap::getLightMatrix(DirectionalLight l)
{
	XMVECTOR position = XMLoadFloat3(&l.position), direction = XMLoadFloat3(&l.direction), upDirection = XMLoadFloat3(&l.upDirection);
	return XMMatrixLookToLH(position, direction, upDirection) * XMMatrixOrthographicLH(l.width, l.height, 0.1f, 100.f);
}

void ShadowMap::transitionWrite(ComPtr<ID3D12GraphicsCommandList> m_commandList)
{
	CD3DX12_RESOURCE_BARRIER *barriers = new CD3DX12_RESOURCE_BARRIER[m_shadowMaps.size() + m_shadowCubeMaps.size()];

	UINT index = 0;
	for (auto &shadowMap : m_shadowMaps) {
		barriers[index++] = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
	}

	for (auto& shadowMap : m_shadowCubeMaps) {
		barriers[index++] = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
	}

	m_commandList->ResourceBarrier(m_shadowMaps.size() + m_shadowCubeMaps.size(), barriers);

	delete[] barriers;
}

void ShadowMap::transitionRead(ComPtr<ID3D12GraphicsCommandList> m_commandList)
{
	CD3DX12_RESOURCE_BARRIER* barriers = new CD3DX12_RESOURCE_BARRIER[m_shadowMaps.size() + m_shadowCubeMaps.size()];

	UINT index = 0;
	for (auto& shadowMap : m_shadowMaps) {
		barriers[index++] = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
	}

	for (auto& shadowMap : m_shadowCubeMaps) {
		barriers[index++] = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMap.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
	}

	m_commandList->ResourceBarrier(m_shadowMaps.size() + m_shadowCubeMaps.size(), barriers);

	delete[] barriers;
}

void ShadowMap::buildResources()
{
	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	m_shadowMaps.clear();
	m_shadowCubeMaps.clear();

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

	D3D12_RESOURCE_DESC texCubeDesc = texDesc;
	texCubeDesc.DepthOrArraySize = 6;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_format;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	for (UINT i = 0; i < m_pointLights.size(); ++i) {
		ComPtr<ID3D12Resource> shadowCubeMap;
		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&texCubeDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&shadowCubeMap)));

		m_shadowCubeMaps.push_back(shadowCubeMap);
		NAME_D3D12_OBJECT_INDEXED(m_shadowCubeMaps, i);
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
		NAME_D3D12_OBJECT_INDEXED(m_shadowMaps,i);
	}
}

void ShadowMap::buildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	//srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvCubeMapDesc = srvDesc;
	srvCubeMapDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvCubeMapDesc.TextureCube.MipLevels = 1;
	srvCubeMapDesc.TextureCube.MostDetailedMip = 0;
	srvCubeMapDesc.TextureCube.ResourceMinLODClamp = 0.f;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_format;
	dsvDesc.Texture2D.MipSlice = 0;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvCubeMapDesc = dsvDesc;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.ArraySize = 1;
	//dsvDesc.Texture2DArray.FirstArraySlice = 0;
	dsvDesc.Texture2DArray.MipSlice = 0;

	for (auto shadowMap : m_shadowMaps) {
		// Create SRV to resource so we can sample the shadow map in a shader program.
		m_device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, m_hCpuSrv);
		m_hCpuSrv.Offset(m_srvIncrementSize);

		// Create DSV to resource so we can render to the shadow map.
		m_device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, m_hCpuDsv);
		m_hCpuDsv.Offset(m_dsvIncrementSize);
	}

	for (auto shadowCubeMap : m_shadowCubeMaps) {
		// Create SRV to resource so we can sample the shadow map in a shader program.
		m_device->CreateShaderResourceView(shadowCubeMap.Get(), &srvCubeMapDesc, m_hCpuSrv);
		m_hCpuSrv.Offset(m_srvIncrementSize);

		for (int i = 0; i < 6; ++i) {
			dsvDesc.Texture2DArray.FirstArraySlice = i;
			// Create DSV to resource so we can render to the shadow map.
			m_device->CreateDepthStencilView(shadowCubeMap.Get(), &dsvDesc, m_hCpuDsv);
			m_hCpuDsv.Offset(m_dsvIncrementSize);
		}
	}

	// Move descriptors back to the start of the heap
	m_hCpuSrv.Offset((-(INT)m_shadowMaps.size() - (INT)m_shadowCubeMaps.size()) * m_srvIncrementSize);
	m_hCpuDsv.Offset((-(INT)m_shadowMaps.size() - 6 * (INT)m_shadowCubeMaps.size()) * m_dsvIncrementSize);
}

void ShadowMap::addPointLight()
{
	m_pointLights.push_back(PointLight{});
	
	ConstantBuffer cb;

	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Scene Constant Buffer
	{
		auto sceneConstantBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * m_frameCount * 6);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&sceneConstantBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cb.buffer)));

		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(cb.buffer->Map(0, &readRange, reinterpret_cast<void**>(&cb.pBufferStart)));

		cb.buffer->SetName((std::wstring(L"PointLightCB") + std::to_wstring(m_pointLightCBs.size())).c_str());
	}

	m_pointLightCBs.push_back(cb);
}

void ShadowMap::addPointLight(const PointLight& data)
{
	UINT index = m_pointLights.size();
	addPointLight();
	updatePointLight(data, index);
}

void ShadowMap::removePointLight()
{
	m_pointLights.pop_back();
	m_pointLightCBs.pop_back();
	m_shadowCubeMaps.pop_back();
}

void ShadowMap::addDirectionalLight()
{
	m_directionalLights.push_back(DirectionalLight{});

	ConstantBuffer cb;

	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Scene Constant Buffer
	{
		auto sceneConstantBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * m_frameCount);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&sceneConstantBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cb.buffer)));

		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(cb.buffer->Map(0, &readRange, reinterpret_cast<void**>(&cb.pBufferStart)));

		cb.buffer->SetName((std::wstring(L"PointLightCB") + std::to_wstring(m_pointLightCBs.size())).c_str());
	}

	m_directionalLightCBs.push_back(cb);
}

void ShadowMap::addDirectionalLight(const DirectionalLight& data)
{
	UINT index = m_directionalLights.size();
	addDirectionalLight();
	updateDirectionalLight(data, index);
}

void ShadowMap::removeDirectionalLight()
{
	m_directionalLights.pop_back();
	m_directionalLightCBs.pop_back();
	m_shadowMaps.pop_back();
}

void ShadowMap::updatePointLight(const PointLight& data, UINT32 index)
{
	m_pointLights[index] = data;
}

void ShadowMap::updateDirectionalLight(const DirectionalLight& data, UINT32 index)
{
	m_directionalLights[index] = data;
	XMVECTOR position = XMLoadFloat3(&data.position), 
		direction = XMLoadFloat3(&data.direction), 
		upDirection = XMLoadFloat3(&data.upDirection);
	/*XMStoreFloat4x4(&m_directionalLights[index].shadowTransform, XMMatrixLookToLH(position, direction, upDirection) * XMMatrixOrthographicLH(data.width, data.height, 0.1f, 100.f));*/	
	XMStoreFloat3(&m_directionalLights[index].direction, XMVector3Normalize(XMLoadFloat3(&m_directionalLights[index].direction)));
	XMStoreFloat3(&m_directionalLights[index].upDirection, XMVector3Normalize(XMLoadFloat3(&m_directionalLights[index].upDirection)));

	m_directionalLights[index].shadowTransform = getLightMatrix(m_directionalLights[index]);
}

void ShadowMap::updateConstantBuffers(UINT frameIndex)
{
	// Update point light CBs
	for (UINT i = 0; i < m_pointLights.size(); ++i) {
		for (UINT j = 0; j < 6; ++j) {
			SceneConstantBuffer cb;
			ZeroMemory(&cb, sizeof(cb));
			cb.view_proj_matrix = getLightMatrix(m_pointLights[i], j); // Only the view_proj matrix matters for shadow pass

			memcpy(m_pointLightCBs[i].pBufferStart + (frameIndex * 6 + j) * sizeof(SceneConstantBuffer), &cb, sizeof(cb));
		}
	}

	// Update directional light CBs
	for (UINT i = 0; i < m_directionalLights.size(); ++i) {
		SceneConstantBuffer cb;
		ZeroMemory(&cb, sizeof(cb));
		cb.view_proj_matrix = getLightMatrix(m_directionalLights[i]);

		memcpy(m_directionalLightCBs[i].pBufferStart + frameIndex * sizeof(SceneConstantBuffer), &cb, sizeof(cb));
	}
}

void ShadowMap::setPointConstantBuffer(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootArgument, UINT frameIndex, UINT lightIndex, UINT passIndex)
{
	m_commandList->SetGraphicsRootConstantBufferView(rootArgument,
		m_pointLightCBs[lightIndex].buffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * (frameIndex * 6 + passIndex));
}

void ShadowMap::setDirectionalConstantBuffer(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootArgument, UINT frameIndex, UINT lightIndex)
{
	m_commandList->SetGraphicsRootConstantBufferView(rootArgument,
		m_directionalLightCBs[lightIndex].buffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frameIndex);
}
