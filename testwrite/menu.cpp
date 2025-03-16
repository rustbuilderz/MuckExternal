#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <thread>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DirectX11 Globals
IDXGISwapChain* pSwapChain = nullptr;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;

// Window Globals
HWND hWnd;
bool showMenu = true;
bool enableWriteMemory = false;
int currentPage = 0;


LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void InitIMGUI(HWND hWnd);
void RenderIMGUI();
void CleanupIMGUI();
void CleanupDeviceD3D();
bool CreateDeviceD3D(HWND hWnd);
void WriteMemoryLoop(bool enable); 

void CreateWindowAndRun() {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL),
                        NULL, NULL, NULL, NULL, L"IMGUI_Menu", NULL };
    RegisterClassExW(&wc);

    hWnd = CreateWindowExW(WS_EX_TOPMOST, L"IMGUI_Menu", L"",
        WS_POPUP, 100, 100, 800, 450,  
        NULL, NULL, wc.hInstance, NULL);

    if (!hWnd) return;

    if (!CreateDeviceD3D(hWnd)) {
        CleanupDeviceD3D();
        return;
    }

    InitIMGUI(hWnd);
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    MSG msg;
    while (true) {
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                CleanupIMGUI();
                return;
            }
        }

        float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
        pContext->ClearRenderTargetView(pRenderTargetView, clear_color);

        RenderIMGUI();
        pSwapChain->Present(1, 0);
    }
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pContext
    );

    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    pBackBuffer->Release();

    return true;
}

void CleanupDeviceD3D() {
    if (pRenderTargetView) { pRenderTargetView->Release(); pRenderTargetView = nullptr; }
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
    if (pContext) { pContext->Release(); pContext = nullptr; }
    if (pDevice) { pDevice->Release(); pDevice = nullptr; }
}

void InitIMGUI(HWND hWnd) {
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

void RenderIMGUI() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (showMenu) {
        ImGui::Begin("IMGUI Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        if (ImGui::Button("Page 1")) currentPage = 0;
        ImGui::SameLine();
        if (ImGui::Button("Page 2")) currentPage = 1;

        ImGui::Separator();

        if (currentPage == 0) {
            ImGui::Text("This is Page 1");
            ImGui::Button("Button on Page 1");
        }
        else if (currentPage == 1) {
            ImGui::Text("This is Page 2");
            ImGui::Button("Button on Page 2");

            
            std::thread memoryThread;
            bool memoryThreadRunning = false;

            if (ImGui::Checkbox("Enable WriteMemoryLoop", &enableWriteMemory)) {
                if (enableWriteMemory && !memoryThreadRunning) {
                    memoryThread = std::thread(WriteMemoryLoop, true);  
                    memoryThread.detach(); 
                    memoryThreadRunning = true;
                }
                else if (!enableWriteMemory && memoryThreadRunning) {
                    memoryThread = std::thread(WriteMemoryLoop, false); 
                    memoryThread.detach();
                    memoryThreadRunning = false;
                }
            }

        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CleanupIMGUI() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_KEYDOWN:
        if (wParam == VK_INSERT) showMenu = !showMenu;
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        break;
    case WM_NCHITTEST:
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(hWnd, &cursor);
        if (cursor.y < 30)
            return HTCAPTION;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

