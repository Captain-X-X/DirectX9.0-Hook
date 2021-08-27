#include <Windows.h>
#include <iostream>
#include "detours.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")

typedef HRESULT(__stdcall* endScene)(IDirect3DDevice9* pDevice);
endScene pEndScene;
//
HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) 
{
	// Code after hooking here
	std::cout << "[*] End Scene Hooked" << std::endl;

	return pEndScene(pDevice); 
}

void HookEndScene()
{
	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
	{
		return;
	}
	D3DPRESENT_PARAMETERS d3dparams = { 0 };
	d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dparams.hDeviceWindow = GetForegroundWindow();
	d3dparams.Windowed = true;
	IDirect3DDevice9* pDevice = nullptr;

	HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		d3dparams.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dparams, &pDevice);
	if (FAILED(result) || !pDevice)
	{
		HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			d3dparams.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dparams, &pDevice);
		if (FAILED(result) || !pDevice)
		{
			pD3D->Release();
			return;
		}
	}
	void** vTable = *reinterpret_cast<void***>(pDevice);
	pEndScene = (endScene)DetourFunction((PBYTE)vTable[42], (PBYTE)hookedEndScene);
	pDevice->Release();
	pD3D->Release();
}

DWORD WINAPI Menue(HINSTANCE hModule)
{
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	HookEndScene();

	while (true)
	{
		Sleep(50);
		if (GetAsyncKeyState(VK_NUMPAD0))
		{
			DetourRemove((PBYTE)pEndScene, (PBYTE)hookedEndScene); 
			break;
		}
	}
	Sleep(1000);
	FreeConsole();
	fclose(fp);
	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}