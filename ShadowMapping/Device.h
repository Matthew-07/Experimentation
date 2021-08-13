#pragma once

using namespace DirectX;
using namespace Microsoft::WRL;

class Device
{
public:
	Device() {}
	void init(D3D_FEATURE_LEVEL featureLevel);
	void init(D3D_FEATURE_LEVEL featureLevel, ComPtr<IDXGIFactory7> factory);

	ComPtr<IDXGIFactory6> getFactory() { return m_factory; }
	ComPtr<ID3D12Device8> getDevice() { return m_device; }
	
	ID3D12Device* operator->() { return m_device.Get(); }

private:
	ComPtr<ID3D12Device8> m_device;
	ComPtr<IDXGIFactory7> m_factory;
};

