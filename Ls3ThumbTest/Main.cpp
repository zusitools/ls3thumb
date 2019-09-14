// Code taken from http://www.directxtutorial.com/Lesson.aspx?lessonid=9-4-8
// and comments removed

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "user32.lib")

#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>

#include "Ls3File.h"
#include "Ls3FileModificationDate.h"
#include "LsFileReader.h"
#include "Ls3FileReader.h"
#include "Ls3FileRenderer.h"

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600

LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;

void initD3D(HWND hWnd);
void cleanD3D(void);
void init_graphics(void);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"Ls3ThumbTestClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL, L"Ls3ThumbTestClass", L"Test for LS3 thumbnailer",
		WS_OVERLAPPEDWINDOW, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	initD3D(hWnd);

	FILETIME lastModified;
	SYSTEMTIME systemTime;
	Ls3FileModificationDate::GetLastModificationDate(L"Z:\\z\\inforiese.ls3", &lastModified);
	FileTimeToSystemTime(&lastModified, &systemTime);

	MSG msg;
	//auto ls3File = LsFileReader::readLs3File(L"Z:\\z\\wuerfel.ls");
	auto ls3File = LsFileReader::readLs3File(L"C:\\DB_517.ls");
	SIZE windowSize;
	windowSize.cx = SCREEN_WIDTH;
	windowSize.cy = SCREEN_HEIGHT;

	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		(new Ls3FileRenderer())->RenderScene(*ls3File, windowSize, d3ddev);
		Sleep(500);
	}

	cleanD3D();

	return msg.wParam;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);
}


void cleanD3D(void)
{
	d3ddev->Release();
	d3d->Release();
}