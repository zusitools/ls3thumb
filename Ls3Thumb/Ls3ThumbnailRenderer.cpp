#include "stdafx.h"
#include "Ls3ThumbnailRenderer.h"
#include "Ls3FileRenderer.h"

#define TRY(action) if (FAILED(hr = action)) { return hr; }


Ls3ThumbnailRenderer::Ls3ThumbnailRenderer(HINSTANCE hInstance)
	: m_hInstance(hInstance)
{
}


Ls3ThumbnailRenderer::~Ls3ThumbnailRenderer()
{
}


HRESULT Ls3ThumbnailRenderer::RenderLs3File(HBITMAP &resultBitmap, Ls3File &file, SIZE &size)
{
	HRESULT hr;

	HWND windowHandle = 0;
	TRY(this->CreateHiddenWindow(windowHandle));
	TRY(this->InitDirect3D(windowHandle, size));
	TRY((new Ls3FileRenderer())->RenderScene(file, size, this->m_d3ddev));
	TRY(this->ReadImageFromDirect3D(resultBitmap));

	this->CleanUpDirect3D();
	DestroyWindow(windowHandle);

	return S_OK;
}

HRESULT Ls3ThumbnailRenderer::CreateHiddenWindow(HWND &handle)
{
	HRESULT hr;

	// Create a window class for our hidden window
	WNDCLASSEX wc;
	wc.hInstance = m_hInstance;
	wc.lpszClassName = (LPCWSTR) L"Ls3ThumbShlExtHiddenWindow";
	wc.lpfnWndProc = DefWindowProc;
	wc.style = 0;
	wc.cbSize = sizeof (WNDCLASSEX);
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hCursor = NULL;
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = NULL;

	TRY(RegisterClassEx(&wc));

	// Create the window itself
	handle = CreateWindowEx(0,
		(LPCWSTR) L"Ls3ThumbShlExtHiddenWindow",
		(LPCWSTR) L"Hidden Window",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		100, 100,
		NULL,
		NULL,
		m_hInstance,
		NULL);

	if (handle == NULL)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT Ls3ThumbnailRenderer::InitDirect3D(HWND hWnd, SIZE &backBufferSize)
{
	m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_DONOTWAIT;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferWidth = backBufferSize.cx;
	d3dpp.BackBufferHeight = backBufferSize.cy;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	return m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&m_d3ddev);
}

void Ls3ThumbnailRenderer::CleanUpDirect3D()
{
	m_d3ddev->Release();
	m_d3ddev = NULL;
	m_d3d->Release();
	m_d3d = NULL;
}

HRESULT Ls3ThumbnailRenderer::ReadImageFromDirect3D(HBITMAP &phBmpBitmap)
{
	HRESULT hr;
	IDirect3DSurface9 *renderTarget;
	hr = m_d3ddev->GetRenderTarget(0, &renderTarget);
	if (!renderTarget || FAILED(hr))
	{
		return hr;
	}

	D3DSURFACE_DESC rtDesc;
	renderTarget->GetDesc(&rtDesc);

	IDirect3DSurface9 *resolvedSurface;
	if (rtDesc.MultiSampleType != D3DMULTISAMPLE_NONE)
	{
		TRY(m_d3ddev->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &resolvedSurface, NULL));
		TRY(hr = m_d3ddev->StretchRect(renderTarget, NULL, resolvedSurface, NULL, D3DTEXF_NONE));
		renderTarget = resolvedSurface;
	}

	IDirect3DSurface9 *offscreenSurface;
	TRY(m_d3ddev->CreateOffscreenPlainSurface(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DPOOL_SYSTEMMEM, &offscreenSurface, NULL));

	hr = m_d3ddev->GetRenderTargetData(renderTarget, offscreenSurface);
	if (SUCCEEDED(hr))
	{
		// Here we have data in offscreenSurface.
		D3DLOCKED_RECT lr;
		RECT rect;
		rect.left = 0;
		rect.right = rtDesc.Width;
		rect.top = 0;
		rect.bottom = rtDesc.Height;
		// Lock the surface to read pixels
		hr = offscreenSurface->LockRect(&lr, &rect, D3DLOCK_READONLY);
		if (SUCCEEDED(hr))
		{
			BITMAPINFO bmInfo;
			memset(&bmInfo.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = rtDesc.Width;
			bmInfo.bmiHeader.biHeight = rtDesc.Height;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 32;

			HDC pDC = GetDC(NULL);
			HDC tmpDC = CreateCompatibleDC(pDC);
			UINT32 *pPixels;
			HBITMAP hBmpThumbnail = CreateDIBSection(tmpDC, &bmInfo,
				DIB_RGB_COLORS, (void**) &pPixels, 0, 0);

			// Pointer to data is lt.pBits, each row is
			// lr.Pitch bytes apart (often it is the same as width*bpp, but
			// can be larger if driver uses padding)
			LONG surfaceOffset = 0;
			BYTE *data = (BYTE*) lr.pBits;
			for (int i = 0; i < rtDesc.Height; i++)
			{
				for (int j = 0; j < rtDesc.Width; j++)
				{
					pPixels[(rtDesc.Height - 1 - i) * rtDesc.Width + j] = RGB(
						data[surfaceOffset + j * 4 + 0],
						data[surfaceOffset + j * 4 + 1],
						data[surfaceOffset + j * 4 + 2]);
				}

				surfaceOffset += lr.Pitch;
			}

			phBmpBitmap = hBmpThumbnail;

			offscreenSurface->UnlockRect();
			hr = S_OK;
		}
	}

	return hr;
}
