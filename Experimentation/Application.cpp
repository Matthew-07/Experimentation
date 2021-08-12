#include "stdafx.h"
#include "Application.h"

void Application::init()
{
    if (!Create(L"D3D12Experimentation", WS_OVERLAPPEDWINDOW, NULL, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height)) {
        printf("CreateWindow error: %d\r\n", GetLastError());
        return;
    }
    m_device.init(D3D_FEATURE_LEVEL_11_1);
    createSwapChain();

    createDescriptorHeaps();
    createFrameResources();

    m_pipeline.init(m_device.getDevice());
    loadAssets();

    // Initalise with camera position (-2, 1, -1) looking at the origin
    m_camera.init(XMVectorSet(2.f, 2.f, 4.f, 1.f), XMVectorZero() - XMVectorSet(2.f, 2.f, 4.f, 0.f));

    ShowWindow(m_hwnd, SW_SHOW);
}

void Application::run()
{
    MSG msg{};

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

void Application::createSwapChain()
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    //bool allowTearing;
    //m_tearingSupport = false;
    //HRESULT hr = m_device.getFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

    //m_tearingSupport = SUCCEEDED(hr) && allowTearing;

    //// It is recommended to always use the tearing flag when it is available.
    //swapChainDesc.Flags = m_tearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_device.getFactory()->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    //m_swapChainEvent = m_swapChain->GetFrameLatencyWaitableObject();
}

void Application::createDescriptorHeaps()
{
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    // Describe and create a depth stencil view (DSV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Describe and create a shader resource view (SRV) and constant 
        // buffer view (CBV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    //cbvSrvHeapDesc.NumDescriptors = 3; // 2 for scene and object constant buffers + 1 for texture SRV
    cbvSrvHeapDesc.NumDescriptors = 1; // Actually, I think the CBVs don't need to go in descriptor heaps since I'm using root descriptors rather than descriptor tables
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
    NAME_D3D12_OBJECT(m_cbvSrvHeap);

    //// Describe and create a sampler descriptor heap.
    //D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    //samplerHeapDesc.NumDescriptors = 1;
    //samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    //samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));

    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Application::createFrameResources()
{
    // Create render target views
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV and a command allocator for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {

            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);

            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
        }
    }

    // Create the depth stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        auto texture2DResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
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

    //ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    NAME_D3D12_OBJECT(m_commandList);
}

void Application::loadAssets()
{
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    monkey.loadFromFile("Pawn.obj");
    //monkey = Mesh::GenerateGrid(10, 10, 0.5f, 0.5f);
    monkey.scheduleUpload(m_device.getDevice(), m_commandList);

    // Load the texture
    ComPtr<ID3D12Resource> textureUploadHeap;

    {
        ScratchImage scratchImage;
        //LoadFromDDSFile(L"texture.DDS", DDS_FLAGS_NONE, nullptr, scratchImage);
        LoadFromDDSFile(L"PawnTexture.DDS", DDS_FLAGS_NONE, nullptr, scratchImage);

        const Image *image = scratchImage.GetImage(1, 0, 0);

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

        UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, subresourceCount, &textureData);
        m_commandList->ResourceBarrier(1, &transitionDesc);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = image->format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    //// Create the sampler
    //{
    //    // Describe and create a sampler.
    //    D3D12_SAMPLER_DESC samplerDesc = {};
    //    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    //    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.MinLOD = 0;
    //    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    //    samplerDesc.MipLODBias = 0.0f;
    //    samplerDesc.MaxAnisotropy = 1;
    //    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    //    m_device->CreateSampler(&samplerDesc, m_samplerHeap->GetCPUDescriptorHandleForHeapStart());
    //}

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
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

    monkey.finaliseUpload();
}

void Application::update()
{
    // Wait for previous frame
    {
        const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

        //m_frameIndex = (m_frameIndex + 1) % FrameCount;

        if (m_frameFenceValues[m_frameIndex] != 0 && m_frameFenceValues[m_frameIndex] > lastCompletedFence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_frameFenceValues[m_frameIndex], m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    if (key_left || key_a) m_camera.left(0.01f);
    if (key_right || key_d) m_camera.right(0.01f);
    if (key_up || key_w) m_camera.forward(0.01f);
    if (key_down || key_s) m_camera.backward(0.01f);
    if (key_space) m_camera.up(0.01f);
    if (key_shift) m_camera.down(0.01f);

    // Update Constant Buffers
    {
        XMMATRIX model = XMMatrixIdentity(), view = m_camera.getViewMatrix(), proj = m_camera.getProjMatrix();

        ObjectConstantBuffer objectCB;
        XMStoreFloat4x4(&objectCB.model_matrix, model);
        //XMStoreFloat4x4(&objectCB.mvp_matrix, XMMatrixTranspose(model * view * proj));
        //XMStoreFloat4x4(&objectCB.mvp_matrix, model * view * proj);
        XMStoreFloat4x4(&objectCB.mvp_matrix, XMMatrixMultiply(view, proj));
        XMStoreFloat4x4(&objectCB.mvp_normalMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, model)));
        objectCB.alpha = 10.f;
        objectCB.k_s = 0.4f;

        SceneConstantBuffer sceneCB;
        sceneCB.ambient_color = { 1.f, 1.f, 1.f };
        sceneCB.ambient_intensity = 0.1f;

        //XMStoreFloat4(&sceneCB.look_direction, m_camera.getLookDirection());
        XMStoreFloat4(&sceneCB.camera_position, m_camera.getPosition());
        //XMStoreFloat4(&sceneCB.look_direction, XMVector3Normalize(m_camera.getLookDirection()));

        for (int i = 0; i < 3; ++i) {
            sceneCB.lightSources[i].intensity = 0.f;
            sceneCB.lightSources[i].color = { 0.f, 0.f, 0.f };
            sceneCB.lightSources[i].position = { 0.f, 0.f, 0.f, 1.f };
        }

        sceneCB.lightSources[0].intensity = 10.f;
        sceneCB.lightSources[0].color = { 1.f, 1.f, 1.f };
        sceneCB.lightSources[0].position = { 4.f, 2.f, 2.f, 1.f };

        //sceneCB.lightSources[1].intensity = 10.f;
        //sceneCB.lightSources[1].color = { 1.f, 1.f, 1.f };
        //sceneCB.lightSources[1].position = { -8.f, -4.f, 2.f, 1.f };

        m_pipeline.setObjectConstantBuffer(objectCB);
        m_pipeline.setSceneConstantBuffer(sceneCB);
        m_pipeline.updateConstantBuffers(m_frameIndex);
    }
}

void Application::render()
{
    //PIXBeginEvent(m_commandQueue.Get(), 0, L"Render");

    // Record all the commands we need to render the scene into the command list.
    populateCommandLists();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    //PIXEndEvent(m_commandQueue.Get());

    // Present and update the frame index for the next frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Signal and increment the fence value.
    m_frameFenceValues[m_frameIndex] = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
    m_fenceValue++;

    //m_frameIndex = (m_frameIndex + 1) % FrameCount;
}

void Application::populateCommandLists()
{
    // Command list allocators can only be reset when the associated
   // command lists have finished execution on the GPU; apps should use
   // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipeline.getPSO().Get()));

    //m_pipeline.setPipelineState(m_commandList, m_frameIndex, m_cbvSrvHeap, m_samplerHeap);
    m_pipeline.setPipelineState(m_commandList, m_frameIndex, m_cbvSrvHeap, m_cbvSrvDescriptorSize);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &transitionDesc);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Render monkey
    monkey.draw(m_commandList);

    // Indicate that the back buffer will now be used to present.
    transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &transitionDesc);

    ThrowIfFailed(m_commandList->Close());
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

        m_camera.yawRight(dx * 0.005f);
        m_camera.pitch(dy * 0.005f);

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
    return L"D3D12_Experimentation";
}
