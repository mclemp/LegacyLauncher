#include "WindowHelper.h"

#include "../Panorama/PanoramaRenderer.h"
#include <thread>

//readme.txt

#ifndef GET_X_PARAM
#define GET_X_PARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_PARAM
#define GET_Y_PARAM(lp) ((int)(short)HIWORD(lp))
#endif
extern "C" {
    extern unsigned char MojanglesTTF_compressed_data[35186];
    extern unsigned int MojanglesTTF_compressed_size;
}
WindowHelper::WindowHelper(const char* WindowName, const char* WindowKlassName) {
    this->WindowName = WindowName;
    this->WindowKlassName = WindowKlassName;
}

bool WindowHelper::StartWindow(HINSTANCE instance) {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        OutputDebugStringA("WARNING: could not set DPI awareness");
    }


    WNDCLASSEXA window_class = { 0 };
    {
        window_class.cbSize = sizeof(window_class);
        window_class.lpszClassName = this->WindowKlassName;
        window_class.lpfnWndProc = win32_custom_title_bar_example_window_callback;
        window_class.style = CS_HREDRAW | CS_VREDRAW;
    }
    RegisterClassExA(&window_class);

    this->windowHandle = CreateWindowExA(WS_EX_APPWINDOW, this->WindowKlassName, this->WindowName, (WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE), CW_USEDEFAULT, CW_USEDEFAULT, 800, 500, 0, 0, instance, 0);
    linkedWindows[this->windowHandle] = this;

    if (!CreateDeviceD3D(this->windowHandle))
    {
        CleanupDeviceD3D();
        ::UnregisterClassA(window_class.lpszClassName, window_class.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(this->windowHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(this->windowHandle);

    std::thread render([this]() { this->ImGuiRenderThread(); });

    render.detach();

    while (!renderDone) {
        if (renderDone) break;
        
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                renderDone = true;
        }
        Sleep(100);
    }

    return 0;
}

void WindowHelper::ImGuiRenderThread() {
#ifdef USING_PANORAMA
    PanoramaRenderer panorama = PanoramaRenderer();
    HMODULE module = GetModuleHandleA(NULL);
    panorama.Initialize(module, g_pd3dDevice, 800, 500);
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; //disable imguis config
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(this->windowHandle);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = 0;
    config.OversampleV = 0;

    io.Fonts->Clear(); // clear fonts if you loaded some before (even if only default one was loaded)

    io.Fonts->AddFontFromMemoryCompressedTTF((void*)MojanglesTTF_compressed_data, MojanglesTTF_compressed_size, 32);

    while (!renderDone)
    {
        if (renderDone) break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RECT title_bar_rect = win32_titlebar_rect(this->windowHandle);

        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(title_bar_rect.left, title_bar_rect.top), ImVec2(title_bar_rect.right, title_bar_rect.bottom), ImGui::GetColorU32(TitleColor));


        CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(this->windowHandle, &title_bar_rect);

        CustomTitleBarHoveredButton title_bar_hovered_button = (CustomTitleBarHoveredButton)GetWindowLongPtrW(this->windowHandle, GWLP_USERDATA);

        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(button_rects.minimize.left, button_rects.minimize.top), ImVec2(button_rects.minimize.right, button_rects.minimize.bottom), ImGui::GetColorU32(((title_bar_hovered_button == CustomTitleBarHoveredButton_Minimize) ? ButtonHoverColor : ButtonColor)));
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(button_rects.minimize.left + 15, button_rects.minimize.bottom / 2), ImVec2(button_rects.minimize.right - 15, button_rects.minimize.bottom / 2), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 1.25);

        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(button_rects.close.left, button_rects.close.top), ImVec2(button_rects.close.right, button_rects.close.bottom), ImGui::GetColorU32(((title_bar_hovered_button == CustomTitleBarHoveredButton_Close) ? CloseButtonColor : ButtonColor)));

        ImVec2 CloseCenter = ImVec2(button_rects.close.left - ((button_rects.close.left - button_rects.close.right) / 2), button_rects.close.bottom - ((button_rects.close.bottom - button_rects.close.top) / 2));

        ImGui::GetForegroundDrawList()->AddLine(ImVec2(CloseCenter.x - 5, CloseCenter.y - 5), ImVec2(CloseCenter.x + 5.5, CloseCenter.y + 5.5), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 1.25);
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(CloseCenter.x + 5.5, CloseCenter.y - 5.5), ImVec2(CloseCenter.x - 5, CloseCenter.y + 5), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 1.25);


        ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), 20, ImVec2(5, 5), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), this->WindowName);

#ifdef USING_PANORAMA
        panorama.Render(g_pd3dDeviceContext, ImGui::GetTime());

        ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)panorama.GetSRV(), ImVec2(1, 1), ImVec2(ImGui::GetIO().DisplaySize.x - 2, ImGui::GetIO().DisplaySize.y - 2));
#endif

        ImGui::SetNextWindowPos(ImVec2(0, title_bar_rect.bottom));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - title_bar_rect.bottom));
        ImGui::Begin("BackgroundContent", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetWindowFontScale(0.5f);

            if (onBackgroundDraw != NULL) onBackgroundDraw();
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(0, title_bar_rect.bottom));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - title_bar_rect.bottom));
        ImGui::Begin("WindowContent", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetWindowFontScale(0.75f);

            if (onContentDraw != NULL) onContentDraw();
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { WindowColor.x * WindowColor.w, WindowColor.y * WindowColor.w, WindowColor.z * WindowColor.w, WindowColor.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(this->windowHandle);
    ::UnregisterClassA(window_class.lpszClassName, window_class.hInstance);
}

std::unordered_map<HWND, WindowHelper*> WindowHelper::linkedWindows;

void WindowHelper::setWindowSize(HWND windowHandle, UINT width, UINT height)
{
    auto it = linkedWindows.find(windowHandle);
    if (it != linkedWindows.end()) {
        WindowHelper* helper = it->second;
        helper->g_ResizeHeight = height;
        helper->g_ResizeWidth = width;
    }
}


// Helper functions

bool WindowHelper::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void WindowHelper::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void WindowHelper::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void WindowHelper::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}


static int win32_dpi_scale(int value, UINT dpi) {
    return (int)((float)value * dpi / 96);
}

static void set_menu_item_state(HMENU menu, MENUITEMINFO* menuItemInfo, UINT item, bool enabled) {
    menuItemInfo->fState = enabled ? MF_ENABLED : MF_DISABLED;
    SetMenuItemInfo(menu, item, false, menuItemInfo);
}

// Adopted from:
// https://github.com/oberth/custom-chrome/blob/master/source/gui/window_helper.hpp#L52-L64
RECT WindowHelper::win32_titlebar_rect(HWND handle) {
    SIZE title_bar_size = { 0 };
    const int top_and_bottom_borders = 2;
    HTHEME theme = OpenThemeData(handle, L"WINDOW");
    UINT dpi = GetDpiForWindow(handle);
    GetThemePartSize(theme, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &title_bar_size);
    CloseThemeData(theme);

    int height = win32_dpi_scale(title_bar_size.cy, dpi) + top_and_bottom_borders;

    RECT rect;
    GetClientRect(handle, &rect);
    rect.bottom = rect.top + height;
    return rect;
}

CustomTitleBarButtonRects WindowHelper::win32_get_title_bar_button_rects(HWND handle, const RECT* title_bar_rect) {
    UINT dpi = GetDpiForWindow(handle);
    CustomTitleBarButtonRects button_rects;
    // Sadly SM_CXSIZE does not result in the right size buttons for Win10
    int button_width = win32_dpi_scale(43, dpi);
    button_rects.close = *title_bar_rect;
    button_rects.close.top = 0;

    button_rects.close.left = button_rects.close.right - button_width;
    button_rects.minimize = button_rects.close;
    button_rects.minimize.left -= button_width;
    button_rects.minimize.right -= button_width;
    return button_rects;
}

static void win32_center_rect_in_rect(RECT* to_center, const RECT* outer_rect) {
    int to_width = to_center->right - to_center->left;
    int to_height = to_center->bottom - to_center->top;
    int outer_width = outer_rect->right - outer_rect->left;
    int outer_height = outer_rect->bottom - outer_rect->top;

    int padding_x = (outer_width - to_width) / 2;
    int padding_y = (outer_height - to_height) / 2;

    to_center->left = outer_rect->left + padding_x;
    to_center->top = outer_rect->top + padding_y;
    to_center->right = to_center->left + to_width;
    to_center->bottom = to_center->top + to_height;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WindowHelper::win32_custom_title_bar_example_window_callback(HWND handle, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(handle, message, w_param, l_param))
        return true;

    CustomTitleBarHoveredButton title_bar_hovered_button =
        (CustomTitleBarHoveredButton)GetWindowLongPtrW(handle, GWLP_USERDATA);

    

    switch (message) {
        // Handling this event allows us to extend client (paintable) area into the title bar region
        // The information is partially coming from:
        // https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
        // Most important paragraph is:
        //   To remove the standard window frame, you must handle the WM_NCCALCSIZE message,
        //   specifically when its wParam value is TRUE and the return value is 0.
        //   By doing so, your application uses the entire window region as the client area,
        //   removing the standard frame.
    case WM_SIZE:
        if (w_param == SIZE_MINIMIZED)
            return 0;

        WindowHelper::setWindowSize(handle, (UINT)LOWORD(l_param), (UINT)HIWORD(l_param));
        return 0;
    case WM_SYSCOMMAND:
        if ((w_param & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_NCCALCSIZE: {
        if (!w_param) return DefWindowProc(handle, message, w_param, l_param);
        UINT dpi = GetDpiForWindow(handle);

        int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
        int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
        int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

        NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)l_param;
        RECT* requested_client_rect = params->rgrc;

        requested_client_rect->right -= frame_x + padding;
        requested_client_rect->left += frame_x + padding;
        requested_client_rect->bottom -= frame_y + padding;

        return 0;
    }
    case WM_CREATE: {
        // Inform the application of the frame change to force redrawing with the new
        // client area that is extended into the title bar
        SetWindowPos(
            handle, NULL,
            0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER
        );
        break;
    }
    case WM_ACTIVATE: {
        RECT title_bar_rect = win32_titlebar_rect(handle);
        InvalidateRect(handle, &title_bar_rect, FALSE);
        return DefWindowProc(handle, message, w_param, l_param);
    }
    case WM_NCHITTEST: {
        // Let the default procedure handle resizing areas
        LRESULT hit = DefWindowProc(handle, message, w_param, l_param);
        switch (hit) {
        case HTNOWHERE:
        case HTRIGHT:
        case HTLEFT:
        case HTTOPLEFT:
        case HTTOP:
        case HTTOPRIGHT:
        case HTBOTTOMRIGHT:
        case HTBOTTOM:
        case HTBOTTOMLEFT: {
            return hit;
        }
        }

        // Looks like adjustment happening in NCCALCSIZE is messing with the detection
        // of the top hit area so manually fixing that.
        UINT dpi = GetDpiForWindow(handle);
        int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
        int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        POINT cursor_point = { 0 };
        cursor_point.x = GET_X_PARAM(l_param);
        cursor_point.y = GET_Y_PARAM(l_param);
        ScreenToClient(handle, &cursor_point);
        if (cursor_point.y > 0 && cursor_point.y < frame_y + padding) {
            return HTTOP;
        }

        // Since we are drawing our own caption, this needs to be a custom test
        if (cursor_point.y < win32_titlebar_rect(handle).bottom) {
            return HTCAPTION;
        }

        return HTCLIENT;
    }
    case WM_PAINT: {
        
        break;
    }
                 // Track when mouse hovers each of the title bar buttons to draw the highlight correctly
    case WM_NCMOUSEMOVE: {
        POINT cursor_point;
        GetCursorPos(&cursor_point);
        ScreenToClient(handle, &cursor_point);

        RECT title_bar_rect = win32_titlebar_rect(handle);
        CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(handle, &title_bar_rect);

        CustomTitleBarHoveredButton new_hovered_button = CustomTitleBarHoveredButton_None;
        if (PtInRect(&button_rects.close, cursor_point)) {
            new_hovered_button = CustomTitleBarHoveredButton_Close;
        }
        else if (PtInRect(&button_rects.minimize, cursor_point)) {
            new_hovered_button = CustomTitleBarHoveredButton_Minimize;
        }
        if (new_hovered_button != title_bar_hovered_button) {
            // You could do tighter invalidation here but probably doesn't matter
            InvalidateRect(handle, &button_rects.close, FALSE);
            InvalidateRect(handle, &button_rects.minimize, FALSE);

            SetWindowLongPtrW(handle, GWLP_USERDATA, (LONG_PTR)new_hovered_button);
        }
        return DefWindowProc(handle, message, w_param, l_param);
    }
                       // If the mouse gets into the client area then no title bar buttons are hovered
                       // so need to reset the hover state
    case WM_MOUSEMOVE: {
        if (title_bar_hovered_button) {
            RECT title_bar_rect = win32_titlebar_rect(handle);
            // You could do tighter invalidation here but probably doesn't matter
            InvalidateRect(handle, &title_bar_rect, FALSE);
            SetWindowLongPtrW(handle, GWLP_USERDATA, (LONG_PTR)CustomTitleBarHoveredButton_None);
        }
        return DefWindowProc(handle, message, w_param, l_param);
    }
                     // Handle mouse down and mouse up in the caption area to handle clicks on the buttons
    case WM_NCLBUTTONDOWN: {
        // Clicks on buttons will be handled in WM_NCLBUTTONUP, but we still need
        // to remove default handling of the click to avoid it counting as drag.
        //
        // Ideally you also want to check that the mouse hasn't moved out or too much
        // between DOWN and UP messages.
        if (title_bar_hovered_button) {
            return 0;
        }
        // Default handling allows for dragging and double click to maximize
        return DefWindowProc(handle, message, w_param, l_param);
    }
                         // Map button clicks to the right messages for the window
    case WM_NCLBUTTONUP: {
        if (title_bar_hovered_button == CustomTitleBarHoveredButton_Close) {
            PostMessageW(handle, WM_CLOSE, 0, 0);
            return 0;
        }
        else if (title_bar_hovered_button == CustomTitleBarHoveredButton_Minimize) {
            ShowWindow(handle, SW_MINIMIZE);
            return 0;
        }
        return DefWindowProc(handle, message, w_param, l_param);
    }
    case WM_NCRBUTTONUP: {
        if (w_param == HTCAPTION) {
            BOOL const isMaximized = IsZoomed(handle);
            MENUITEMINFO menu_item_info = MENUITEMINFO();
            menu_item_info.cbSize = sizeof(menu_item_info);
            menu_item_info.fMask = MIIM_STATE;
            HMENU const sys_menu = GetSystemMenu(handle, false);
            set_menu_item_state(sys_menu, &menu_item_info, SC_RESTORE, isMaximized);
            set_menu_item_state(sys_menu, &menu_item_info, SC_MOVE, !isMaximized);
            set_menu_item_state(sys_menu, &menu_item_info, SC_SIZE, !isMaximized);
            set_menu_item_state(sys_menu, &menu_item_info, SC_MINIMIZE, true);
            set_menu_item_state(sys_menu, &menu_item_info, SC_MAXIMIZE, !isMaximized);
            set_menu_item_state(sys_menu, &menu_item_info, SC_CLOSE, true);
            BOOL const result = TrackPopupMenu(sys_menu, TPM_RETURNCMD, GET_X_PARAM(l_param), GET_Y_PARAM(l_param), 0, handle, NULL);
            if (result != 0) {
                PostMessage(handle, WM_SYSCOMMAND, result, 0);
            }
        }
        return DefWindowProc(handle, message, w_param, l_param);
    }
    case WM_SETCURSOR: {
        // Show an arrow instead of the busy cursor
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(handle, message, w_param, l_param);
}
