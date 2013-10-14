#pragma once

#pragma comment (lib, "d3d9.lib")

#include "windows.h"
#include "d3d9.h"
#include "Ls3File.h"

class Ls3ThumbnailRenderer
{
public:
	Ls3ThumbnailRenderer(HINSTANCE hInstance);
	~Ls3ThumbnailRenderer();

	HRESULT RenderLs3File(HBITMAP &resultBitmap, Ls3File &file, SIZE &size);

private:
	HRESULT CreateHiddenWindow(HWND &handle);
	HRESULT InitDirect3D(HWND hWnd, SIZE &backBufferSize);
	void CleanUpDirect3D();
	
	HRESULT ReadImageFromDirect3D(HBITMAP &phBmpBitmap);

	HINSTANCE m_hInstance;
	LPDIRECT3D9 m_d3d;
	LPDIRECT3DDEVICE9 m_d3ddev;
};

