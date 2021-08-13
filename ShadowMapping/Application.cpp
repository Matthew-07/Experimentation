#include "stdafx.h"
#include "Application.h"

const float Application::ClearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

Application::Application(UINT width, UINT height, UINT windowWidth, UINT windowHeight) :
        m_sceneViewport(0.0f, 0.0f, 0.0f, 0.0f),
        m_sceneScissorRect(0, 0, 0, 0),
        m_postViewport(0.0f, 0.0f, 0.0f, 0.0f),
        m_postScissorRect(0, 0, 0, 0),
        m_rectangle(std::array<ElementDescriptor, 2>{ { {"position", _FLOAT32, 3, 0}, { "texture", _FLOAT32, 2, 12 }} }.data(), 2)
{
    m_width = width;
    m_height = height;
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    init();
    run();
}

void Application::init()
{
    if (!Create(L"D3D12Experimentation", WS_OVERLAPPEDWINDOW, NULL, CW_USEDEFAULT, CW_USEDEFAULT, m_windowWidth, m_windowHeight)) {
        printf("CreateWindow error: %d\r\n", GetLastError());
        return;
    }

    m_device.init(D3D_FEATURE_LEVEL_11_0);

    createSwapchain();
    createScene();
    createDescriptorHeaps();

    createWindowSizeDependentResources();
    createResolutionDependentResources();

    createRootSignatures();
    createPipelineState();

    ShowWindow(m_hwnd, SW_SHOW);
}

void Application::run()
{

}

void Application::updatePostViewAndScissor() {
    float viewWidthRatio = static_cast<float>(m_width) / m_windowWidth;
    float viewHeightRatio = static_cast<float>(m_height) / m_windowHeight;

    float x = 1.0f;
    float y = 1.0f;

    if (viewWidthRatio < viewHeightRatio)
    {
        // The scaled image's height will fit to the viewport's height and 
        // its width will be smaller than the viewport's width.
        x = viewWidthRatio / viewHeightRatio;
    }
    else
    {
        // The scaled image's width will fit to the viewport's width and 
        // its height may be smaller than the viewport's height.
        y = viewHeightRatio / viewWidthRatio;
    }

    m_postViewport.TopLeftX = m_width * (1.0f - x) / 2.0f;
    m_postViewport.TopLeftY = m_height * (1.0f - y) / 2.0f;
    m_postViewport.Width = x * m_width;
    m_postViewport.Height = y * m_height;

    m_postScissorRect.left = static_cast<LONG>(m_postViewport.TopLeftX);
    m_postScissorRect.right = static_cast<LONG>(m_postViewport.TopLeftX + m_postViewport.Width);
    m_postScissorRect.top = static_cast<LONG>(m_postViewport.TopLeftY);
    m_postScissorRect.bottom = static_cast<LONG>(m_postViewport.TopLeftY + m_postViewport.Height);
}

void Application::createWindowSizeDependentResources()
{
    updatePostViewAndScissor();

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);
        }
    }
}

void Application::createResolutionDependentResources()
{
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // Set up the scene viewport and scissor rect to match the current scene rendering resolution.
    {
        m_sceneViewport.Width = static_cast<float>(m_width);
        m_sceneViewport.Height = static_cast<float>(m_height);

        m_sceneScissorRect.right = static_cast<LONG>(m_width);
        m_sceneScissorRect.bottom = static_cast<LONG>(m_height);
    }

    // Update post-process viewport and scissor rectangle.
    updatePostViewAndScissor();

    // Create RTV for the intermediate render target.
    {
        D3D12_RESOURCE_DESC swapChainDesc = m_renderTargets[m_frameIndex]->GetDesc();
        const CD3DX12_CLEAR_VALUE clearValue(swapChainDesc.Format, ClearColor);
        const CD3DX12_RESOURCE_DESC renderTargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            swapChainDesc.Format,
            m_width,
            m_height,
            1u, 1u,
            swapChainDesc.SampleDesc.Count,
            swapChainDesc.SampleDesc.Quality,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            D3D12_TEXTURE_LAYOUT_UNKNOWN, 0u);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameCount, m_rtvDescriptorSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &renderTargetDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&m_intermediateRenderTarget)));
        m_device->CreateRenderTargetView(m_intermediateRenderTarget.Get(), nullptr, rtvHandle);
        NAME_D3D12_OBJECT(m_intermediateRenderTarget);
    }

    // Create SRV for the intermediate render target.
    m_device->CreateShaderResourceView(m_intermediateRenderTarget.Get(), nullptr, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Application::createSwapchain()
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    NAME_D3D12_OBJECT(m_commandQueue);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    // It is recommended to always use the tearing flag when it is available.
    swapChainDesc.Flags = m_tearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_device.getFactory()->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    if (m_tearingSupport)
    {
        // When tearing support is enabled we will handle ALT+Enter key presses in the
        // window message loop rather than let DXGI handle it by calling SetFullscreenState.
        m_device.getFactory()->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    }

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Application::createDescriptorHeaps()
{
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount + 1; // +1 for intermediate render target
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Describe and create a depth stencil view (DSV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1 + m_shadowMap.getDescriptorCount();
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));    
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Describe and create a shader resource view (SRV) and constant 
        // buffer view (CBV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = 2; // One srv to use for texturing and one for the post shaders
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
    NAME_D3D12_OBJECT(m_cbvSrvHeap);
}

void Application::createScene()
{
    m_rectangle.initAsGrid(2, 2, 2.f, 2.f, -1.f, -1.f);

    Object monkey("monkey.mesh", m_device, FrameCount);
    monkey.setPosition(XMVectorZero());
    monkey.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey);

    ElementDescriptor descriptors[3] = {
            { "position", _FLOAT32, 3, 0 },
            { "normal", _FLOAT32, 3, 12 },
            { "tex", _FLOAT32, 2, 24 }
    };
    Mesh planeMesh{ descriptors, 3 };
    planeMesh.initAsGrid(2, 2, 10.f, 10.f);

    Object plane(std::move(planeMesh), "plane", m_device, FrameCount);
    plane.setPosition(XMVectorSet(-5.f, -5.f, 0.f, 1.f));
    plane.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    plane.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(plane);

    PointLight p;
    p.color = { 1.f, 1.f, 1.f };
    p.intensity = 4.f;
    p.position = { 3.f, 2.f, 1.f };
    m_shadowMap.addPointLight(p);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 2> Application::createStaticSamplers()
{
    CD3DX12_STATIC_SAMPLER_DESC diffuseSampler = {};
    diffuseSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    diffuseSampler.ShaderRegister = 1;
    diffuseSampler.RegisterSpace = 0;
    diffuseSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    diffuseSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    diffuseSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    diffuseSampler.MipLODBias = 0;
    diffuseSampler.MaxAnisotropy = 0;
    diffuseSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    diffuseSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    diffuseSampler.MinLOD = 0.0f;
    diffuseSampler.MaxLOD = D3D12_FLOAT32_MAX;    
    diffuseSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    const CD3DX12_STATIC_SAMPLER_DESC shadowSampler(
        2, // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
        0.0f,                               // mipLODBias
        16,                                 // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

    return {
        diffuseSampler, shadowSampler
    };
}

void Application::createRootSignatures()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Scene root signature
    {
        CD3DX12_DESCRIPTOR_RANGE shadowMapTable;
        shadowMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 2, 0);

        CD3DX12_DESCRIPTOR_RANGE shadowCubeMapTable;
        shadowCubeMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 5, 0);

        CD3DX12_DESCRIPTOR_RANGE textureTable;
        textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

        CD3DX12_ROOT_PARAMETER slotRootParameter[5];
        slotRootParameter[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
        slotRootParameter[1].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[2].InitAsDescriptorTable(1, &shadowCubeMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[3].InitAsDescriptorTable(1, &shadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[4].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

        auto staticSamplers = createStaticSamplers();

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(5, slotRootParameter,
            (UINT)staticSamplers.size(), staticSamplers.data(),
            rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
        if (FAILED(hr)) {
            OutputDebugStringA((char*)error->GetBufferPointer());
            ThrowIfFailed(hr);
        }
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_sceneRootSignature)));
    }

    // Post root signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        // We don't modify the SRV in the post-processing command list after
        // SetGraphicsRootDescriptorTable is executed on the GPU so we can use the default
        // range behavior: D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        // Allow input layout and pixel shader access and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // Create a sampler.
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_postRootSignature)));
        NAME_D3D12_OBJECT(m_postRootSignature);
    }
}

void Application::createPipelineState()
{
    // Define the vertex input layouts.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    ComPtr<ID3D10Blob> basicVS, basicPS;
    basicVS = loadBinary("basic_vertex_shader.cso");
    basicPS = loadBinary("basic_pixel_shader.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC basicPsoDesc;

    // PSO for basic objects.
    ZeroMemory(&basicPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    basicPsoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    basicPsoDesc.pRootSignature = m_sceneRootSignature.Get();
    basicPsoDesc.VS = CD3DX12_SHADER_BYTECODE(basicVS.Get());
    basicPsoDesc.PS = CD3DX12_SHADER_BYTECODE(basicPS.Get());
    basicPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    basicPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    basicPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    basicPsoDesc.SampleMask = UINT_MAX;
    basicPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basicPsoDesc.NumRenderTargets = 1;
    basicPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    basicPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    basicPsoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&basicPsoDesc, IID_PPV_ARGS(&m_basicPipelineState)));

    ComPtr<ID3D10Blob> shadowsVS, shadowsPS;
    shadowsVS = loadBinary("shadows_vertex_shader.cso");
    //shadowsPS = loadBinary("shadows_pixel_shader.cso");
    

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowsPsoDesc = basicPsoDesc;
    shadowsPsoDesc.pRootSignature = m_sceneRootSignature.Get();
    basicPsoDesc.VS = CD3DX12_SHADER_BYTECODE(shadowsVS.Get());
    //basicPsoDesc.PS = D3D12_SHADER_BYTECODE(shadowsPS.Get());
    basicPsoDesc.PS = CD3DX12_SHADER_BYTECODE{};
    shadowsPsoDesc.RasterizerState.DepthBias = 100000;
    shadowsPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    shadowsPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    // Shadow map pass does not have a render target.
    shadowsPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    shadowsPsoDesc.NumRenderTargets = 0;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&shadowsPsoDesc, IID_PPV_ARGS(&m_shadowsPipelineState)));

    // Define the vertex input layouts.
    D3D12_INPUT_ELEMENT_DESC postInputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    ComPtr<ID3D10Blob> postVS, postPS;
    postVS = loadBinary("post_vertex_shader.cso");
    postPS = loadBinary("post_pixel_shader.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC postPsoDesc = basicPsoDesc;
    postPsoDesc.InputLayout = { postInputElementDescs, _countof(postInputElementDescs) };
    postPsoDesc.pRootSignature = m_postRootSignature.Get();
    postPsoDesc.VS = CD3DX12_SHADER_BYTECODE(postVS.Get());
    postPsoDesc.PS = CD3DX12_SHADER_BYTECODE(postPS.Get());
    postPsoDesc.DepthStencilState.DepthEnable = FALSE;
    postPsoDesc.DepthStencilState.StencilEnable = FALSE;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&postPsoDesc, IID_PPV_ARGS(&m_postPipelineState)));
    NAME_D3D12_OBJECT(m_postPipelineState);
}

ComPtr<ID3D10Blob> Application::loadBinary(std::string filename)
{
    std::ifstream fs(filename, std::ios::binary);

    if (!fs) {
        throw std::exception((std::string("Failed to open file ") + filename + ".").c_str());
    }

    // Get file size
    fs.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fs.tellg();
    fs.seekg(0, std::ios_base::beg);

    // Create blob large enough to contain entire file
    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size,
        blob.GetAddressOf()));

    // Copy file to blob
    fs.read((char*)blob->GetBufferPointer(), size);

    fs.close();
    return blob;
}

void update() {

}

void render() {
    
}

LRESULT Application::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

PCWSTR Application::ClassName() const
{
    return L"ShadowMapping";
}
