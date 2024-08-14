#include "pch.h"
#include "Hooks.h"
#include "Menu.h"

bool menu_open = false;
bool setup = false;
HWND window = nullptr;
WNDCLASSEX windowClass = {};
WNDPROC originalWindowProcess = nullptr;
LPDIRECT3DDEVICE9 device = nullptr;
LPDIRECT3D9 d3d9 = nullptr;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		menu_open = !menu_open;
	}

	if (menu_open && ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
	{
		return 1L;
	}
	return CallWindowProc(originalWindowProcess, window, msg, wParam, lParam);
}

void DestroyDirectX()
{
	if (device)
	{
		device->Release();
		device = NULL;
	}
	if (d3d9)
	{
		d3d9->Release();
		d3d9 = NULL;
	}
}
void Setup()
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "HackClass1";
	windowClass.hIconSm = NULL;
	if (!RegisterClassEx(&windowClass))
	{
		throw std::runtime_error("Failed to create window class.");
	}

	window = CreateWindow(windowClass.lpszClassName, "hack wnd", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, windowClass.hInstance, 0);
	if (!window)
	{
		throw std::runtime_error("Failed to create window.");
	}

	const auto handle = GetModuleHandle("d3d9.dll");
	if (!handle)
		throw std::runtime_error("Failed to get D3D9 handle.");

	using CreateFn = LPDIRECT3D9(__stdcall*)(UINT);
	const auto create = reinterpret_cast<CreateFn>(GetProcAddress(handle, "Direct3DCreate9"));
	if (!create)
		throw std::runtime_error("Failed to get D3D9 function 'Direct3DCreate9'.");

	d3d9 = create(D3D_SDK_VERSION);
	if (!d3d9)
		throw std::runtime_error("Failed to setup D3D9.");

	D3DPRESENT_PARAMETERS d3dparams = { };
	d3dparams.BackBufferWidth = 0;
	d3dparams.BackBufferHeight = 0;
	d3dparams.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dparams.BackBufferCount = 0;
	d3dparams.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dparams.MultiSampleQuality = NULL;
	d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dparams.hDeviceWindow = window;
	d3dparams.Windowed = 1;
	d3dparams.EnableAutoDepthStencil = 0;
	d3dparams.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	d3dparams.Flags = NULL;
	d3dparams.FullScreen_RefreshRateInHz = 0;
	d3dparams.PresentationInterval = 0;
	if (d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &d3dparams, &device) < 0)
	{
		throw std::runtime_error("Failed to setup dx9.");
	}

	if (window)
	{
		DestroyWindow(window);
		UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
	}
	printf("Window, WindowClass & DirectX9 Setup and ready for hook!\n");
}

void Destroy()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWindowProcess));
	DestroyDirectX();
}

constexpr void* VirtualFunction(void* thisptr, size_t index)
{
	return (*static_cast<void***>(thisptr))[index];
}
using EndSceneFn = long(__thiscall*)(void*, IDirect3DDevice9*);
EndSceneFn EndSceneOriginal = nullptr;
long __stdcall callback_EndScene(IDirect3DDevice9* pDevice)
{
	//static const auto returnAddress = _ReturnAddress();
	//const auto result = EndSceneOriginal(device, device);
	//if (_ReturnAddress() == returnAddress)
	//{
	//	return result;
	//}

	if (!setup)
	{
		auto params = D3DDEVICE_CREATION_PARAMETERS{};
		pDevice->GetCreationParameters(&params);
		window = params.hFocusWindow;
		originalWindowProcess = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX9_Init(pDevice);
		setup = true;
	}
	if (menu_open)
	{
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		Menuc::RenderImGuiMenu(menu_open);

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	return EndSceneOriginal(pDevice, pDevice);;
}

using ResetFn = HRESULT(__thiscall*)(void*, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
ResetFn ResetOriginal = nullptr;
HRESULT __stdcall Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = ResetOriginal(pDevice, pDevice, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

void HookEndScene()
{
	try
	{
		Setup();
		if (MH_Initialize())
		{
			throw std::runtime_error("Unable to initilize minhook!");
		}
		if (MH_CreateHook(VirtualFunction(device, 42), &callback_EndScene, reinterpret_cast<void**>(&EndSceneOriginal)))
		{
			throw std::runtime_error("Unable to hook EndScene()!");
		}
		if (MH_CreateHook(VirtualFunction(device, 16), &Reset, reinterpret_cast<void**>(&ResetOriginal)))
		{
			throw std::runtime_error("Unable to hook Reset()!");
		}
		if (MH_EnableHook(MH_ALL_HOOKS))
		{
			throw std::runtime_error("Unable to enable hooks!");
		}
		DestroyDirectX();
	}
	catch (const std::exception& error)
	{
		MessageBeep(MB_ICONERROR);
		MessageBox(0, error.what(), "Error!", MB_OK | MB_ICONEXCLAMATION);
		goto UNLOAD;
	}

	printf("MinHook Initilized!");
	while (!GetAsyncKeyState(VK_END) & 1)
	    std::this_thread::sleep_for(std::chrono::microseconds(200));

UNLOAD:
	Destroy();
	return;
}

namespace SEHooks
{
    void InitilizeHooks()
    {
        FILE* console;
        AllocConsole();
        AttachConsole(GetProcessId(GetCurrentProcess()));
        freopen_s(&console, "CONOUT$", "w", stdout);
        SetConsoleTitleA("Simple HL2: Console");
		// hook D3DX9
		HookEndScene();
		// close the console output handle
        std::fclose(console);
    }

    void DisableHooks()
    {
        printf("Unhooking & exiting thread!\n");
		// unhook D3Dx9
		MH_DisableHook(MH_ALL_HOOKS);
		MH_RemoveHook(MH_ALL_HOOKS);
		MH_Uninitialize();
		// close console
        FreeConsole(); // This way you can close the console without closing hl2
    }
}