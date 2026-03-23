#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "d3d11.lib")

#include <stdbool.h>
#include <d3d11.h>
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include <unordered_map>
#include <functional>
#include <atomic>

//readme.txt

typedef struct {
	RECT close;
	RECT minimize;
} CustomTitleBarButtonRects;

typedef enum {
	CustomTitleBarHoveredButton_None,
	CustomTitleBarHoveredButton_Minimize,
	CustomTitleBarHoveredButton_Close,
} CustomTitleBarHoveredButton;

class WindowHelper {
public:
	WindowHelper(const char* WindowName, const char* WindowKlassName);
	bool StartWindow(HINSTANCE instance);

	void ImGuiRenderThread();

	std::function<void()> onContentDraw;
	std::function<void()> onBackgroundDraw;

	std::atomic<bool> renderDone = false;

	ImVec4 TitleColor = ImVec4(0.2, 0.2, 0.2, 1);
	ImVec4 WindowColor = ImVec4(0.2, 0.2, 0.2, 1);
	ImVec4 ButtonColor = ImVec4();
	ImVec4 ButtonHoverColor = ImVec4(0.25, 0.25, 0.25, 1);
	ImVec4 CloseButtonColor = ImVec4(1, 0, 0, 1);

	HWND getWindowHandle() { return this->windowHandle; };
	static RECT win32_titlebar_rect(HWND handle);
protected:
	HWND windowHandle;

	static void setWindowSize(HWND windowHandle, UINT width, UINT height);
	static std::unordered_map<HWND, WindowHelper*> linkedWindows;

private:
	const char* WindowName;
	const char* WindowKlassName;

	WNDCLASSEXA window_class = { 0 };

	ID3D11Device* g_pd3dDevice = nullptr;
	ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
	IDXGISwapChain* g_pSwapChain = nullptr;
	bool g_SwapChainOccluded = false;
	UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
	ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

	bool CreateDeviceD3D(HWND hWnd);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	static CustomTitleBarButtonRects win32_get_title_bar_button_rects(HWND handle, const RECT* title_bar_rect);
	static LRESULT win32_custom_title_bar_example_window_callback(HWND handle, UINT message, WPARAM w_param, LPARAM l_param);

	bool started = false;
};