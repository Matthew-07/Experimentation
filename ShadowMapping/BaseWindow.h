#pragma once

template <class WindowClass>
class BaseWindow
{
public:
	bool Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
	);

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	HWND m_hwnd;

	virtual LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	virtual PCWSTR  ClassName() const = 0;

};

template<class WindowClass>
inline bool BaseWindow<WindowClass>::Create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu) {
	WNDCLASS wc = { 0 };

	wc.lpfnWndProc = WindowClass::WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = ClassName();

	RegisterClass(&wc);
#ifdef _DEBUG
	printf("RegisterClass Error: %d\n", GetLastError());
#endif

	m_hwnd = CreateWindowEx(
		dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
		nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
	);

	return (m_hwnd ? TRUE : FALSE);
}

template<class WindowClass>
LRESULT CALLBACK BaseWindow<WindowClass>::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowClass* pWindow;
	if (msg == WM_NCCREATE)
	{
		CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
		pWindow = (WindowClass*)createStruct->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pWindow);
	}
	else {
		pWindow = (WindowClass*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	if (pWindow != nullptr) {
		return pWindow->handleMessage(hwnd, msg, wParam, lParam);
	}
	else {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}