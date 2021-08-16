#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

class Cubemap
{
public:
	void loadFromFile(const wchar_t* filename, ComPtr<ID3D12Device> m_device, ComPtr<ID3D12GraphicsCommandList> m_commandList);
	void finaliseUpload() { m_textureUploadHeap.Reset(); }
	
	void createDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);

	CD3DX12_GPU_DESCRIPTOR_HANDLE SRV() { return m_hGpuSrv; }

private:
	ComPtr<ID3D12Resource> m_textureCube;
	ComPtr<ID3D12Resource> m_textureUploadHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;

	TexMetadata m_metaData;

	ComPtr<ID3D12Device> m_device;
};

