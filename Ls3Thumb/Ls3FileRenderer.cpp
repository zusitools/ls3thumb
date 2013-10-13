#include "stdafx.h"
#include "Ls3FileRenderer.h"

#define ZUSIFVF (D3DFVF_XYZ /* | D3DFVF_NORMAL | D3DFVF_DIFFUSE */ \
	/* | D3DFVF_SPECULAR | D3DFVF_TEX1 */)

#define TRY(action) if (FAILED(hr = action)) { return hr; }

Ls3FileRenderer::Ls3FileRenderer(HINSTANCE hInstance)
	: m_hInstance(hInstance)
{
}


Ls3FileRenderer::~Ls3FileRenderer()
{
}


HRESULT Ls3FileRenderer::RenderLs3File(HBITMAP &resultBitmap, Ls3File &file, SIZE &size)
{
	HRESULT hr;

	HWND windowHandle = 0;
	TRY(this->CreateHiddenWindow(windowHandle));
	TRY(this->InitDirect3D(windowHandle, size));
	TRY(this->RenderScene(file, size));
	TRY(this->ReadImageFromDirect3D(resultBitmap));

	this->CleanUpDirect3D();
	DestroyWindow(windowHandle);

	return S_OK;
}

HRESULT Ls3FileRenderer::CreateHiddenWindow(HWND &handle)
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
	HWND hwnd = CreateWindowEx(0,
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

	if (hwnd == NULL)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT Ls3FileRenderer::InitDirect3D(HWND hWnd, SIZE &backBufferSize)
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

HRESULT Ls3FileRenderer::RenderScene(Ls3File &file, SIZE &size)
{
	HRESULT hr;

	D3DXVECTOR3 cameraPosition(5.0f, 5.0f, 5.0f);
	D3DXVECTOR3 cameraTarget(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 cameraUp(0.0f, 1.0f, 0.0f);
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtLH(&viewMatrix, &cameraPosition, &cameraTarget, &cameraUp);

	TRY(m_d3ddev->SetTransform(D3DTS_VIEW, &viewMatrix));

	// Setup the projection matrix
	D3DXMATRIX projectionMatrix;
	D3DXMatrixPerspectiveFovLH(&projectionMatrix, D3DX_PI / 4, (float) size.cx / (float) size.cy, 0.1f, 100.0f);

	TRY(m_d3ddev->SetTransform(D3DTS_PROJECTION, &projectionMatrix));

	// Turn off the 3D lighting and turn on Z buffering
	TRY(m_d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE));
	TRY(m_d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE));

	TRY(m_d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(116, 165, 210), 1.0f, 0));
	TRY(m_d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0));
	TRY(m_d3ddev->BeginScene());
	TRY(m_d3ddev->SetFVF(ZUSIFVF));

	VOID* pData;
	LPDIRECT3DVERTEXBUFFER9 pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 pIndexBuffer;

	for (auto it = file.subsets.begin(); it != file.subsets.end(); it++)
	{
		const Ls3MeshSubset subset = *it;
		const size_t numVertices = subset.vertices.size();
		const size_t numFaceIndices = subset.faceIndices.size();
		const size_t numFaces = numFaceIndices / 3;

		if (numFaces == 0) {
			continue;
		}

		TRY(m_d3ddev->CreateVertexBuffer(
			numVertices * sizeof(ZUSIVERTEX),
			0, ZUSIFVF, D3DPOOL_MANAGED, &pVertexBuffer, NULL));

		pVertexBuffer->Lock(0, 0, (void**) &pData, 0);
		memcpy(pData, subset.vertices.data(), numVertices * sizeof(ZUSIVERTEX));
		pVertexBuffer->Unlock();

		TRY(m_d3ddev->CreateIndexBuffer(
			numFaceIndices * sizeof(UINT32),
			0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &pIndexBuffer, NULL));

		pIndexBuffer->Lock(0, 0, (void**) &pData, 0);
		memcpy(pData, subset.faceIndices.data(), numFaceIndices * sizeof(UINT32));
		pIndexBuffer->Unlock();

		TRY(m_d3ddev->SetIndices(pIndexBuffer));
		TRY(m_d3ddev->SetStreamSource(0, pVertexBuffer, 0, sizeof(ZUSIVERTEX)));
		TRY(m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numFaces));
	}

	TRY(m_d3ddev->EndScene());
	TRY(m_d3ddev->Present(NULL, NULL, NULL, NULL));

	return S_OK;
}

void Ls3FileRenderer::CleanUpDirect3D()
{
	m_d3ddev->Release();
	m_d3ddev = NULL;
	m_d3d->Release();
	m_d3d = NULL;
}

HRESULT Ls3FileRenderer::ReadImageFromDirect3D(HBITMAP &phBmpBitmap)
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
