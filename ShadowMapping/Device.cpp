#include "stdafx.h"
#include "Device.h"

struct DeviceException : std::exception {
    DeviceException() : std::exception("Could not create a device with the required feature level.") {}
};

void Device::init(D3D_FEATURE_LEVEL featureLevel)
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory7> factory;
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
    init(featureLevel, factory);
}

void Device::init(D3D_FEATURE_LEVEL featureLevel, ComPtr<IDXGIFactory7> factory)
{

	m_factory = factory;

	ComPtr<IDXGIAdapter1> hardwareAdaptor;

    ComPtr<IDXGIAdapter1> adapter;
    hardwareAdaptor = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    hardwareAdaptor = adapter.Detach();

    if (hardwareAdaptor == nullptr) {
        throw DeviceException();
    }

    HRESULT hr = D3D12CreateDevice(
        hardwareAdaptor.Get(),
        featureLevel,
        IID_PPV_ARGS(&m_device)
    );

    if (FAILED(hr)) throw DeviceException();
}
