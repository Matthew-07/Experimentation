#pragma once

#include "BaseWindow.h"
#include "Device.h"
#include "Object.h"
#include "ShadowMap.h"
#include "Camera.h"

struct PostVertex
{
	XMFLOAT4 position;
	XMFLOAT2 uv;
};

struct AmbientLight {
	XMFLOAT3 color;
	float intensity;
};

struct SceneConstantBuffer
{
	XMMATRIX view_proj_matrix; // 16

	XMFLOAT3 camera_position; // 3
	float f; // data cannot cross 16 byte boundaries : 1

	AmbientLight ambient; // 4

	PointLight point_lights[3]; // 8 * 3 = 24
	UINT32 number_of_lights; // 1

	float padding[64 - 16 - 3 - 1 - 4 - 24 - 1];
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
	bool m_windowedMode = true;

	Device m_device;

	CD3DX12_VIEWPORT m_sceneViewport;
	CD3DX12_VIEWPORT m_postViewport;
	CD3DX12_RECT m_sceneScissorRect;
	CD3DX12_RECT m_postScissorRect;

	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_intermediateRenderTarget;
	ComPtr<ID3D12Resource> m_depthStencil;

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

	enum SceneRootParameters {
		RootObjectConstantBuffer = 0,
		RootTextureTable = 1,
		RootSceneShadowCubeMapTable = 2,
		RootSceneShadowMapTable = 3,		
		RootSceneConstantBuffer = 4
	};

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;
	UINT m_cbvSrvDescriptorSize;

	Camera m_camera;
	std::vector<Object> m_sceneObjects;
	ShadowMap m_shadowMap;
	ComPtr<ID3D12Resource> m_texture;

	ComPtr<ID3D12Resource> m_sceneConstantBuffer;
	SceneConstantBuffer m_sceneConstantBufferData;
	UINT8* m_pSceneCbvDataBegin;

	Mesh m_rectangle;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[FrameCount];
	UINT64 m_fenceValue;

	bool key_left = false, key_right = false, key_up = false, key_down = false;
	bool key_a = false, key_d = false, key_w = false, key_s = false;
	bool key_space = false, key_shift = false;

	bool paused = false; // pause key

	std::chrono::steady_clock::time_point lastUpdate;

	void updatePostViewAndScissor();
	void createWindowSizeDependentResources();
	void createResolutionDependentResources();

	void createSwapchain();
	void createScene();
	void createDescriptorHeaps();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 2> createStaticSamplers();
	void createRootSignatures();
	void createPipelineState();

	void createCommandObjects();
	void loadAssets();

	// Wait for the GPU to finish with resources for the current frame index
	void waitForFrame();

	// Wait for the GPU to finsh all commands
	void waitForGPU();

	void update();
	void render();
	void populateCommandLists();

	void onWindowSizeChange(UINT width, UINT height);
	void onResolutionChange(UINT width, UINT height);

	ComPtr<ID3D10Blob> loadBinary(std::string filename);
};