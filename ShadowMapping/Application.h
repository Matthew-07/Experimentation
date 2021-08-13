#pragma once

#include "BaseWindow.h"
#include "Device.h"
#include "Object.h"
#include "ShadowMap.h"

struct PostVertex
{
	XMFLOAT4 position;
	XMFLOAT2 uv;
};

class Application : public BaseWindow<Application>
{
public:
	Application(UINT width, UINT height, UINT screenWidth, UINT screenHeight);

	void init();
	void run();

	// Inherited via BaseWindow
	virtual LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual PCWSTR ClassName() const override;

private:
	UINT m_width, m_height;
	UINT m_windowWidth, m_windowHeight;

	static const float ClearColor[4];

	static const UINT FrameCount = 2;
	bool m_tearingSupport;

	Device m_device;

	CD3DX12_VIEWPORT m_sceneViewport;
	CD3DX12_VIEWPORT m_postViewport;
	CD3DX12_RECT m_sceneScissorRect;
	CD3DX12_RECT m_postScissorRect;

	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_intermediateRenderTarget;

	ComPtr<ID3D12CommandAllocator> m_sceneCommandAllocators[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_postCommandAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_sceneCommandList;
	ComPtr<ID3D12GraphicsCommandList> m_postCommandList;
	ComPtr<ID3D12CommandQueue> m_commandQueue;

	ComPtr<ID3D12RootSignature> m_sceneRootSignature;
	ComPtr<ID3D12RootSignature> m_postRootSignature;
	ComPtr<ID3D12PipelineState> m_basicPipelineState;
	ComPtr<ID3D12PipelineState> m_shadowsPipelineState;
	ComPtr<ID3D12PipelineState> m_postPipelineState;

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;
	UINT m_cbvSrvDescriptorSize;

	ComPtr<ID3D12Resource> m_postVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_postVertexBufferView;

	std::vector<Object> m_sceneObjects;
	ShadowMap m_shadowMap;

	Mesh m_rectangle;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[FrameCount];

	void updatePostViewAndScissor();
	void createWindowSizeDependentResources();
	void createResolutionDependentResources();

	void createSwapchain();
	void createScene();
	void createDescriptorHeaps();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 2> createStaticSamplers();
	void createRootSignatures();
	void createPipelineState();

	void update();
	void render();

	ComPtr<ID3D10Blob> loadBinary(std::string filename);
};