#pragma once

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#include "windows.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "Ls3File.h"

class Ls3FileRenderer
{
public:
	Ls3FileRenderer(HINSTANCE hInstance);
	~Ls3FileRenderer();

	HRESULT RenderLs3File(HBITMAP &resultBitmap, Ls3File &file, SIZE &size);

private:
	HRESULT CreateHiddenWindow(HWND &handle);
	HRESULT InitDirect3D(HWND hWnd, SIZE &backBufferSize);
	HRESULT RenderScene(Ls3File &file, SIZE &size);
	void CleanUpDirect3D();
	HRESULT ReadImageFromDirect3D(HBITMAP &phBmpBitmap);

	HINSTANCE m_hInstance;

	LPDIRECT3D9 m_d3d;
	LPDIRECT3DDEVICE9 m_d3ddev;
};
