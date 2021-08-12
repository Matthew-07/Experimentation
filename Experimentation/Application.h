#pragma once

#include "BaseWindow.h"
#include "Pipeline.h"
#include "Device.h"
#include "Mesh.h"
#include "Camera.h"

using namespace DirectX;
using namespace Microsoft::WRL;

class Application : public BaseWindow<Application>
{
public:
	//Application() : monkey("monkey.obj"), m_viewport{ 0.f, 0.f, m_width, m_height }, m_scissorRect{ 0, 0, m_width, m_height } {}
	Application() : m_viewport{ 0.f, 0.f, m_width, m_height }, m_scissorRect{ 0, 0, m_width, m_height } {}

	void init();
	void run();
	
	static const UINT FrameCount = 2;

	// Inherited via BaseWindow
	LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	PCWSTR ClassName() const override;

private:
	// now inherited from BaseWindow
	//HWND m_hwnd = 0; 

	static const UINT m_width = 1280, m_height = 720;

	//bool mousePosInit = false;
	//float mouse_x, mouse_y;

	bool key_left = false, key_right = false, key_up = false, key_down = false;
	bool key_a = false, key_d = false, key_w = false, key_s = false;
	bool key_space = false, key_shift = false;

	Camera m_camera;

	Device m_device;
	Pipeline m_pipeline;
	Mesh monkey;

	ComPtr<ID3D12Resource> m_texture;

	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	ComPtr<IDXGISwapChain3> m_swapChain;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_depthStencil;
	UINT m_frameIndex;

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	UINT m_rtvDescriptorSize;

	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	//ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
	UINT m_cbvSrvDescriptorSize;

	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	UINT64 m_frameFenceValues[FrameCount]{ 0 };

	void createSwapChain();
	void createDescriptorHeaps();
	void createFrameResources();
	void loadAssets();

	void update();
	void render();

	void populateCommandLists();
};

