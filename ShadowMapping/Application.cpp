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

    createCommandObjects();
    loadAssets();

    ShowWindow(m_hwnd, SW_SHOW);
}

void Application::run()
{
    MSG msg{};

    lastUpdate = std::chrono::steady_clock::now();

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT ||
                msg.message == WM_DESTROY ||
                msg.message == WM_CLOSE)
            {
                PostQuitMessage(0);
                break;
            }

            //if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            /*if (!TranslateAccelerator(hWnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            update();
            render();
        }
    }
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
            //swapChainDesc.SampleDesc.Count,
            4,
            //swapChainDesc.SampleDesc.Quality,
            0,
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
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_renderTargets[m_frameIndex]->GetDesc().Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    /*m_device->CreateShaderResourceView(m_intermediateRenderTarget.Get(), nullptr, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());*/
    m_device->CreateShaderResourceView(m_intermediateRenderTarget.Get(), &srvDesc, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create the depth stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        //depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        /*auto texture2DResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);*/
        auto texture2DResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 1, 4, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        ThrowIfFailed(m_device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            //&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_resolution.Width, m_resolution.Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            &texture2DResourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencil)
        ));

        NAME_D3D12_OBJECT(m_depthStencil);

        m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
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
    dsvHeapDesc.NumDescriptors = 1 + m_shadowMap.getDsvDescriptorCount();
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));    
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Describe and create a shader resource view (SRV) and constant 
        // buffer view (CBV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = 2 + m_shadowMap.getSrvDescriptorCount(); // One srv to use for texturing and one for the post shaders plus srvs for shadow mapping
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
    NAME_D3D12_OBJECT(m_cbvSrvHeap);
    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_shadowMap.buildDescriptors(m_cbvSrvDescriptorSize, m_dsvDescriptorSize,
        CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize },
        CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize },
        CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_dsvDescriptorSize });
}

void Application::createScene()
{
    XMVECTOR cameraPosition = XMVectorSet(5.f, 0.f, 1.f, 1.f);
    m_camera.init(cameraPosition, -cameraPosition, XMVectorSet(0.f, 0.f, 1.f, 0.f), XMConvertToRadians(70.f),
        (float) m_windowWidth / m_windowHeight, 0.1f, 100.f);

    m_rectangle.initAsGrid(2, 2, 2.f, 2.f, -1.f, -1.f);

    Object monkey("monkey.mesh", m_device, FrameCount);
    monkey.setPosition(XMVectorZero());
    monkey.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey);

    Object monkey2("monkey.mesh", m_device, FrameCount);
    monkey2.setPosition(XMVectorSet(-2.f, 2.f, 0.f, 0.f));
    monkey2.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey2.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey2);

    Object monkey3("monkey.mesh", m_device, FrameCount);
    monkey3.setPosition(XMVectorSet(-2.f, -2.f, 0.f, 0.f));
    monkey3.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey3.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey3);

    Object monkey4("monkey.mesh", m_device, FrameCount);
    monkey4.setPosition(XMVectorSet(-4.f, 0.f, 0.f, 0.f));
    monkey4.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey4.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey4);

    Object monkey5("monkey.mesh", m_device, FrameCount);
    monkey5.setPosition(XMVectorSet(-4.f, -4.f, 0.f, 0.f));
    monkey5.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey5.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey5);

    Object monkey6("monkey.mesh", m_device, FrameCount);
    monkey6.setPosition(XMVectorSet(-4.f, 4.f, 0.f, 0.f));
    monkey6.setScaling(XMVectorSet(1.f, 1.f, 1.f, 1.f));
    monkey6.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(monkey6);

    ElementDescriptor descriptors[3] = {
            { "position", _FLOAT32, 3, 0 },
            { "normal", _FLOAT32, 3, 12 },
            { "tex", _FLOAT32, 2, 24 }
    };
    Mesh planeMesh{ descriptors, 3 };
    planeMesh.initAsGrid(2, 2, 20.f, 20.f, -10.f, -10.f);

    Object plane(std::move(planeMesh), "plane", m_device, FrameCount);
    plane.setPosition(XMVectorSet(0.f, 0.f, -1.f, 1.f));
    //plane.setScaling(XMVectorSet(-1.f, 1.f, 1.f, 1.f));
    plane.setScaling(XMVectorSet(-1.f, 1.f, 1.f, 1.f));
    plane.setQuaternion(XMQuaternionIdentity());
    m_sceneObjects.push_back(plane);

    m_shadowMap.init(m_device.getDevice().Get(), FrameCount);

    PointLight p;
    p.color = { 1.f, 1.f, 1.f };
    p.intensity = 20.f;
    p.position = { 2.f, 2.f, 3.f };
    m_shadowMap.addPointLight(p);

    m_shadowMap.buildResources();
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
        slotRootParameter[RootObjectConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
        slotRootParameter[RootTextureTable].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[RootSceneShadowCubeMapTable].InitAsDescriptorTable(1, &shadowCubeMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[RootSceneShadowMapTable].InitAsDescriptorTable(1, &shadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[RootSceneConstantBuffer].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

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
    //basicPsoDesc.SampleDesc.Count = 1;
    basicPsoDesc.SampleDesc.Count = 4;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&basicPsoDesc, IID_PPV_ARGS(&m_basicPipelineState)));

    ComPtr<ID3D10Blob> shadowsVS, shadowsPS;
    shadowsVS = loadBinary("shadows_vertex_shader.cso");
    //shadowsPS = loadBinary("shadows_pixel_shader.cso");
    

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowsPsoDesc = basicPsoDesc;
    shadowsPsoDesc.pRootSignature = m_sceneRootSignature.Get();
    shadowsPsoDesc.VS = CD3DX12_SHADER_BYTECODE(shadowsVS.Get());
    shadowsPsoDesc.PS.BytecodeLength = 0;
    shadowsPsoDesc.PS.pShaderBytecode = nullptr;
    //shadowsPsoDesc.RasterizerState.DepthBias =   100000;
    shadowsPsoDesc.RasterizerState.DepthBias = 100;
    shadowsPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    shadowsPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    // Shadow map pass does not have a render target.
    shadowsPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    shadowsPsoDesc.NumRenderTargets = 0;
    shadowsPsoDesc.SampleDesc.Count = 1;
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
    postPsoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&postPsoDesc, IID_PPV_ARGS(&m_postPipelineState)));
    NAME_D3D12_OBJECT(m_postPipelineState);
}

void Application::createCommandObjects()
{
    // Create command allocators for each frame.
    for (UINT n = 0; n < FrameCount; n++)
    {
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[n])));
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_postCommandAllocators[n])));
    }

    // Single-use command allocator and command list for creating resources.
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // Create the command lists.
    {
        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[m_frameIndex].Get(), m_basicPipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandList)));
        NAME_D3D12_OBJECT(m_sceneCommandList);

        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_postCommandAllocators[m_frameIndex].Get(), m_postPipelineState.Get(), IID_PPV_ARGS(&m_postCommandList)));
        NAME_D3D12_OBJECT(m_postCommandList);

        //// Close the command lists.
        //ThrowIfFailed(m_sceneCommandList->Close());
        ThrowIfFailed(m_postCommandList->Close());
    }
}

void Application::loadAssets() {
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    // Load the texture
    ComPtr<ID3D12Resource> textureUploadHeap;

    {
        ScratchImage scratchImage;
        LoadFromDDSFile(L"texture.DDS", DDS_FLAGS_NONE, nullptr, scratchImage);
        //LoadFromDDSFile(L"PawnTexture.DDS", DDS_FLAGS_NONE, nullptr, scratchImage);

        const Image* image = scratchImage.GetImage(1, 0, 0);

        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = image->format;
        textureDesc.Width = image->width;
        textureDesc.Height = image->height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_texture)));

        NAME_D3D12_OBJECT(m_texture);

        const UINT subresourceCount = textureDesc.DepthOrArraySize * textureDesc.MipLevels;
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, subresourceCount);

        auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        ThrowIfFailed(m_device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)));

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the Texture2D.
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = image->pixels;
        textureData.RowPitch = image->rowPitch;
        textureData.SlicePitch = image->slicePitch;

        auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        UpdateSubresources(m_sceneCommandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, subresourceCount, &textureData);
        m_sceneCommandList->ResourceBarrier(1, &transitionDesc);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = image->format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        CD3DX12_CPU_DESCRIPTOR_HANDLE texHandle{ m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize };
        m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, texHandle);
    }

    Object::uploadMeshes(m_device.getDevice(), m_sceneCommandList);
    m_rectangle.scheduleUpload(m_device.getDevice(), m_sceneCommandList);

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_sceneCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_sceneCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        m_fenceValue = 0;
        ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.

        // Signal and increment the fence value.
        const UINT64 fenceToWaitFor = m_fenceValue;
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
        m_fenceValue++;

        // Wait until the fence is completed.
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    // SceneConstantBuffer
    {
        auto sceneConstantBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * Application::FrameCount);

        ThrowIfFailed(m_device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &sceneConstantBufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_sceneConstantBuffer)));

        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pSceneCbvDataBegin)));

        m_sceneConstantBuffer->SetName(L"SceneConstantBuffer");
    }

    Object::finaliseMeshes();
    m_rectangle.finaliseUpload();
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

void Application::update() {
    // Wait for previous frame
    {
        const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

        //m_frameIndex = (m_frameIndex + 1) % FrameCount;

        if (m_fenceValues[m_frameIndex] != 0 && m_fenceValues[m_frameIndex] > lastCompletedFence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    auto timeNow = std::chrono::steady_clock::now();

    float timeElapsed = std::chrono::duration_cast<std::chrono::microseconds>(timeNow - lastUpdate).count();
    lastUpdate = timeNow;

    float distance = 0.000005f * timeElapsed;

    if (key_left || key_a) m_camera.left(distance);
    if (key_right || key_d) m_camera.right(distance);
    if (key_up || key_w) m_camera.forward(distance);
    if (key_down || key_s) m_camera.backward(distance);
    if (key_space) m_camera.up(distance);
    if (key_shift) m_camera.down(distance);

    ZeroMemory(&m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));  

    m_sceneConstantBufferData.number_of_lights = m_shadowMap.getNumberOfPointLights();
    for (int i = 0; i < m_shadowMap.getNumberOfPointLights(); ++i) {
        m_sceneConstantBufferData.point_lights[i] = m_shadowMap.getPointLight(i);
    }
    m_sceneConstantBufferData.ambient.color = { 1.f, 1.f, 1.f };
    m_sceneConstantBufferData.ambient.intensity = 0.1f;
    //m_sceneConstantBufferData.view_proj_matrix = 
    //    XMMatrixLookAtLH(XMVectorSet(5.f, 0.f, 1.f, 1.f), XMVectorZero(), XMVectorSet(0.f, 0.f, 1.f, 0.f)) *
    //    XMMatrixPerspectiveFovLH(XMConvertToRadians(70.f), m_windowWidth / m_windowHeight, 0.1f, 100.f);
    //m_sceneConstantBufferData.camera_position = { 5.f, 0.f, 1.f };
    m_sceneConstantBufferData.view_proj_matrix = m_camera.getViewMatrix() * m_camera.getProjMatrix();
    XMStoreFloat3(&m_sceneConstantBufferData.camera_position, m_camera.getPosition());

    memcpy(m_pSceneCbvDataBegin + m_frameIndex * sizeof(m_sceneConstantBufferData), &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));

    for (auto& object : m_sceneObjects) {
        object.updateConstantBuffer(m_frameIndex);
    }

    m_shadowMap.updateConstantBuffers(m_frameIndex);
}

void Application::render() {

    // Record all the commands we need to render the scene into the command lists.
    populateCommandLists();

    // Execute the command lists.
    ID3D12CommandList* ppCommandLists[] = { m_sceneCommandList.Get(), m_postCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // When using sync interval 0, it is recommended to always pass the tearing
    // flag when it is supported, even when presenting in windowed mode.
    // However, this flag cannot be used if the app is in fullscreen mode as a
    // result of calling SetFullscreenState.
    UINT presentFlags = (m_tearingSupport && m_windowedMode) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(0, presentFlags));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Signal and increment the fence value.
    m_fenceValues[m_frameIndex] = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
    m_fenceValue++;
}

void Application::populateCommandLists() {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_sceneCommandAllocators[m_frameIndex]->Reset());
    ThrowIfFailed(m_postCommandAllocators[m_frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_sceneCommandList->Reset(m_sceneCommandAllocators[m_frameIndex].Get(), m_shadowsPipelineState.Get()));
    ThrowIfFailed(m_postCommandList->Reset(m_postCommandAllocators[m_frameIndex].Get(), m_postPipelineState.Get()));

    m_sceneCommandList->SetGraphicsRootSignature(m_sceneRootSignature.Get());
    m_sceneCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get() };
    m_sceneCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Shadow passes
    {
        PIXBeginEvent(m_sceneCommandList.Get(), 0, "Shadow Pass");

        //m_sceneCommandList->SetPipelineState(m_shadowsPipelineState.Get());

        m_sceneCommandList->RSSetViewports(1, m_shadowMap.getViewport());
        m_sceneCommandList->RSSetScissorRects(1, m_shadowMap.getScissorRect());

        CD3DX12_CPU_DESCRIPTOR_HANDLE m_dsvHandle = m_shadowMap.DSVShadowCubeMaps();

        m_shadowMap.transitionWrite(m_sceneCommandList);

        // Point Lights
        for (UINT i = 0; i < m_shadowMap.getNumberOfPointLights(); ++i) {
            for (UINT j = 0; j < 6; ++j) {
                m_sceneCommandList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

                m_shadowMap.setPointConstantBuffer(m_sceneCommandList, RootSceneConstantBuffer, m_frameIndex, i, j);
                m_sceneCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_dsvHandle);
                
                for (auto& object : m_sceneObjects) {
                    object.draw(m_sceneCommandList, RootObjectConstantBuffer, m_frameIndex);
                }

                m_dsvHandle.Offset(m_dsvDescriptorSize);
            }
        }


        PIXEndEvent(m_sceneCommandList.Get());
    }

    // Main pass
    {
        PIXBeginEvent(m_sceneCommandList.Get(), 0, "Main Pass");

        m_sceneCommandList->SetGraphicsRootConstantBufferView(RootSceneConstantBuffer, m_sceneConstantBuffer->GetGPUVirtualAddress() + m_frameIndex * sizeof(SceneConstantBuffer));

        m_sceneCommandList->SetPipelineState(m_basicPipelineState.Get());

        m_sceneCommandList->RSSetViewports(1, &m_sceneViewport);
        m_sceneCommandList->RSSetScissorRects(1, &m_sceneScissorRect);

        m_shadowMap.transitionRead(m_sceneCommandList);

        // Bind texture
        CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle{ m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize };
        m_sceneCommandList->SetGraphicsRootDescriptorTable(RootTextureTable, texHandle);

        // Bind shadow maps
        if (m_shadowMap.getNumberOfDirectionalLights() > 0)
            m_sceneCommandList->SetGraphicsRootDescriptorTable(RootSceneShadowMapTable, m_shadowMap.SRVShadowMaps());
        if (m_shadowMap.getNumberOfPointLights() > 0)
            m_sceneCommandList->SetGraphicsRootDescriptorTable(RootSceneShadowCubeMapTable, m_shadowMap.SRVShadowCubeMaps());

        // Bind render target and depth buffer
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameCount, m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        m_sceneCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
  
        // Clear render target and depth buffer        
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_sceneCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_sceneCommandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        for (auto& object : m_sceneObjects) {
            object.draw(m_sceneCommandList, RootObjectConstantBuffer, m_frameIndex);
        }

        //m_rectangle.draw(m_sceneCommandList);

        PIXEndEvent(m_sceneCommandList.Get());
    }

    m_sceneCommandList->Close();

    
    // Post pass
    {
        PIXBeginEvent(m_postCommandList.Get(), 0, "Post pass");
        // Set necessary state.
        m_postCommandList->SetGraphicsRootSignature(m_postRootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get() };
        m_postCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // Indicate that the back buffer will be used as a render target and the
        // intermediate render target will be used as a SRV.
        D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
            CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };

        m_postCommandList->ResourceBarrier(_countof(barriers), barriers);

        m_postCommandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        m_postCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_postCommandList->RSSetViewports(1, &m_postViewport);
        m_postCommandList->RSSetScissorRects(1, &m_postScissorRect);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        m_postCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_postCommandList->ClearRenderTargetView(rtvHandle, ClearColor, 0, nullptr);
        //m_postCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        //m_postCommandList->IASetVertexBuffers(0, 1, &m_postVertexBufferView);
        

        PIXBeginEvent(m_postCommandList.Get(), 0, L"Draw texture to screen.");
        //m_postCommandList->DrawInstanced(5, 1, 0, 0);
        m_rectangle.draw(m_postCommandList);
        PIXEndEvent(m_postCommandList.Get());

        // Revert resource states back to original values.
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        m_postCommandList->ResourceBarrier(_countof(barriers), barriers);
        PIXEndEvent(m_postCommandList.Get());
    }

    m_postCommandList->Close();
}

LRESULT Application::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
    {
        POINT center;
        center.x = m_width / 2;
        center.y = m_height / 2;

        ClientToScreen(m_hwnd, &center);

        SetCursorPos(center.x, center.y);

        ShowCursor(false);

        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    case WM_MOUSEMOVE:
    {
        float xPos = GET_X_LPARAM(lParam);
        float yPos = GET_Y_LPARAM(lParam);

        //if (mousePosInit) {
        //    m_camera.pitch((yPos - mouse_y) * 0.005f);
        //    m_camera.yawRight((xPos - mouse_x) * 0.005f);
        //}

        //mouse_x = xPos;
        //mouse_y = yPos;
        //mousePosInit = true;

        float dx = xPos - (m_width / 2.f);
        float dy = yPos - (m_height / 2.f);

        m_camera.yawRight(dx * 0.004f);
        m_camera.pitch(dy * 0.004f);

        POINT center;
        center.x = m_width / 2;
        center.y = m_height / 2;

        ClientToScreen(m_hwnd, &center);

        SetCursorPos(center.x, center.y);

        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case VK_LEFT:
            key_left = true;
            break;
        case VK_RIGHT:
            key_right = true;
            break;
        case VK_UP:
            key_up = true;
            break;
        case VK_DOWN:
            key_down = true;
            break;

        case 0x41: // A
            key_a = true;
            break;
        case 0x44: // D
            key_d = true;
            break;
        case 0x53: // S
            key_s = true;
            break;
        case 0x57: // W
            key_w = true;
            break;

        case VK_SPACE:
            key_space = true;
            break;
        case VK_SHIFT:
            key_shift = true;
            break;
        }
        break;
    }
    case WM_KEYUP:
    {
        switch (wParam) {
        case VK_LEFT:
            key_left = false;
            break;
        case VK_RIGHT:
            key_right = false;
            break;
        case VK_UP:
            key_up = false;
            break;
        case VK_DOWN:
            key_down = false;
            break;

        case 0x41: // A
            key_a = false;
            break;
        case 0x44: // D
            key_d = false;
            break;
        case 0x53: // S
            key_s = false;
            break;
        case 0x57: // W
            key_w = false;
            break;

        case VK_SPACE:
            key_space = false;
            break;
        case VK_SHIFT:
            key_shift = false;
            break;
        }
        break;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

PCWSTR Application::ClassName() const
{
    return L"ShadowMapping";
}
