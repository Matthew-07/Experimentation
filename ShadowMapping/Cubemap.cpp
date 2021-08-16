#include "stdafx.h"
#include "Cubemap.h"

void Cubemap::loadFromFile(const wchar_t* filename, ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> m_commandList)
{
    m_device = device;

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ScratchImage scratchImage;
    LoadFromDDSFile(filename, DDS_FLAGS_NONE, nullptr, scratchImage);
    
    m_metaData = scratchImage.GetMetadata();
    if (m_metaData.arraySize != 6) throw std::exception("Expected Cubemap but did not recieve 6 images.");

    const Image* images = scratchImage.GetImages();
    //const Image* image = scratchImage.GetImage();

    //scratchImage.InitializeCube(m_metaData.format, m_metaData.width, m_metaData.height, 1, m_metaData.mipLevels);

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = m_metaData.mipLevels;
    textureDesc.Format = m_metaData.format;
    textureDesc.Width = m_metaData.width;
    textureDesc.Height = m_metaData.height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 6;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_textureCube)));

    m_textureCube->SetName((std::wstring(L"textureCube_") + std::wstring(filename)).c_str());

    //const UINT subresourceCount = textureDesc.DepthOrArraySize * textureDesc.MipLevels;
    const UINT subresourceCount = scratchImage.GetImageCount();
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textureCube.Get(), 0, subresourceCount);

    auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_textureUploadHeap)));

    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.
    std::vector<D3D12_SUBRESOURCE_DATA> subresources{ subresourceCount };
    for (int i = 0; i < subresourceCount; ++i) {
        subresources[i].pData = images[i].pixels;
        subresources[i].RowPitch = images[i].rowPitch;
        subresources[i].SlicePitch = images[i].slicePitch;
    }

    auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_textureCube.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    UpdateSubresources(m_commandList.Get(), m_textureCube.Get(), m_textureUploadHeap.Get(), 0, 0, subresourceCount, subresources.data());
    m_commandList->ResourceBarrier(1, &transitionDesc);
}

void Cubemap::createDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
    m_hCpuSrv = hCpuSrv;
    m_hGpuSrv = hGpuSrv;

    // Describe and create a SRV for the texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = m_metaData.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = m_metaData.mipLevels;
    m_device->CreateShaderResourceView(m_textureCube.Get(), &srvDesc, m_hCpuSrv);
}
