// Ls3ThumbShlExt.h : Declaration of the CLs3ThumbShlExt

#pragma once

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#include "resource.h"       // main symbols
#include "shobjidl.h"
#include "windows.h"
#include "windowsx.h"
#include "d3d9.h"
#include "d3dx9.h"

#include <memory>

#include "Ls3File.h"
#include "Ls3FileReader.h"

#include "Ls3Thumb_i.h"

#define ZUSIFVF (D3DFVF_XYZ /* | D3DFVF_NORMAL | D3DFVF_DIFFUSE */ \
	/* | D3DFVF_SPECULAR | D3DFVF_TEX1 */)

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CLs3ThumbShlExt

class ATL_NO_VTABLE CLs3ThumbShlExt :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLs3ThumbShlExt, &CLSID_Ls3ThumbShlExt>,
	public ILs3ThumbShlExt,
	public IPersistFile,
	public IExtractImage
{
public:
	CLs3ThumbShlExt()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LS3THUMBSHLEXT)

DECLARE_NOT_AGGREGATABLE(CLs3ThumbShlExt)

BEGIN_COM_MAP(CLs3ThumbShlExt)
	COM_INTERFACE_ENTRY(ILs3ThumbShlExt)
	COM_INTERFACE_ENTRY(IPersistFile)
	COM_INTERFACE_ENTRY(IExtractImage)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	// IPersistFile
	STDMETHOD(GetClassID)(CLSID*) { return E_NOTIMPL; }
	STDMETHOD(IsDirty)()                  { return E_NOTIMPL; }
	STDMETHOD(Save)(LPCOLESTR, BOOL)    { return E_NOTIMPL; }
	STDMETHOD(SaveCompleted)(LPCOLESTR) { return E_NOTIMPL; }
	STDMETHOD(GetCurFile)(LPOLESTR*) { return E_NOTIMPL; }

	STDMETHOD(Load)(LPCOLESTR wszFile, DWORD /*dwMode*/)
	{
		USES_CONVERSION;
		lstrcpyn(m_szFilename, OLE2CT(wszFile), MAX_PATH);
		return S_OK;
	}

	// IExtractImage
	STDMETHOD(Extract)(HBITMAP* phBmpThumbnail)
	{
		HRESULT hr;

		char fileName[MAX_PATH];
		size_t i;
		wcstombs_s(&i, fileName, MAX_PATH, m_szFilename, MAX_PATH);
		
		std::unique_ptr<Ls3File> ls3File(Ls3FileReader::readLs3File(fileName));

		if (ls3File->subsets.size() == 0) {
			return S_OK;
		}

		// Create a window class for our hidden window
		WNDCLASSEX wc;
		wc.hInstance = _AtlBaseModule.GetModuleInstance();
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

		if (FAILED(hr = RegisterClassEx(&wc)))
		{
			return hr;
		}

		HWND hwnd = CreateWindowEx(0,
			(LPCWSTR) L"Ls3ThumbShlExtHiddenWindow",
			(LPCWSTR) L"Hidden Window",
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			m_rgSize.cx, m_rgSize.cy,
			NULL,
			NULL,
			_AtlBaseModule.GetModuleInstance(),
			NULL);

		if (hwnd == NULL)
		{
			return E_FAIL;
		}

		if (FAILED(hr = this->InitDirect3D(hwnd)))
		{
			return hr;
		}

		D3DXVECTOR3 cameraPosition(5.0f, 5.0f, 5.0f);
		D3DXVECTOR3 cameraTarget(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 cameraUp(0.0f, 1.0f, 0.0f);
		D3DXMATRIX viewMatrix;
		D3DXMatrixLookAtLH(&viewMatrix, &cameraPosition, &cameraTarget, &cameraUp);

		m_d3ddev->SetTransform(D3DTS_VIEW, &viewMatrix);

		// Setup the projection matrix
		D3DXMATRIX projectionMatrix;
		D3DXMatrixPerspectiveFovLH(&projectionMatrix, D3DX_PI / 4, (float) m_rgSize.cx / (float) m_rgSize.cy, 0.1f, 100.0f);

		m_d3ddev->SetTransform(D3DTS_PROJECTION, &projectionMatrix);
		m_d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);    // turn off the 3D lighting

		m_d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 165, 210), 1.0f, 0);

		if (FAILED(m_d3ddev->BeginScene())) return E_FAIL;
		if (FAILED(m_d3ddev->SetFVF(ZUSIFVF))) return E_FAIL;

		VOID* pData;

		for (auto it = ls3File->subsets.begin(); it != ls3File->subsets.end(); it++)
		{
			const Ls3MeshSubset subset = *it;
			const size_t numVertices = subset.vertices.size();
			const size_t numFaceIndices = subset.faceIndices.size();
			const size_t numFaces = numFaceIndices / 3;

			if (numFaces == 0) {
				continue;
			}

			if (FAILED(hr = m_d3ddev->CreateVertexBuffer(
				numVertices * sizeof(ZUSIVERTEX),
				0, ZUSIFVF, D3DPOOL_MANAGED, &m_vbuffer, NULL)))
			{
				return hr;
			}
			
			m_vbuffer->Lock(0, 0, (void**) &pData, 0);
			memcpy(pData, subset.vertices.data(), numVertices * sizeof(ZUSIVERTEX));
			m_vbuffer->Unlock();

			if (FAILED(hr = m_d3ddev->CreateIndexBuffer(
				numFaceIndices * sizeof(UINT32),
				0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_ibuffer, NULL)))
			{
				return hr;
			}

			m_ibuffer->Lock(0, 0, (void**) &pData, 0);
			memcpy(pData, subset.faceIndices.data(), numFaceIndices * sizeof(UINT32));
			m_ibuffer->Unlock();

			if (FAILED(hr = m_d3ddev->SetIndices(m_ibuffer))) return hr;
			if (FAILED(hr = m_d3ddev->SetStreamSource(0, m_vbuffer, 0, sizeof(ZUSIVERTEX)))) return hr;
			if (FAILED(hr = m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numFaces))) return hr;
		}

		if (FAILED(m_d3ddev->EndScene())) return E_FAIL;
		if (FAILED(m_d3ddev->Present(NULL, NULL, NULL, NULL))) return E_FAIL;

		this->ReadImageFromDirect3D(phBmpThumbnail);
		this->CleanUpDirect3D();

		DestroyWindow(hwnd);
		return S_OK;
	}

	STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
	{
		// Save requested thumbnail size
		m_rgSize.cx = prgSize->cx;
		m_rgSize.cy = prgSize->cy;
		return S_OK;
	}

	HRESULT CLs3ThumbShlExt::InitDirect3D(HWND hWnd)
	{
		m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));

		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.PresentationInterval = D3DPRESENT_DONOTWAIT;
		d3dpp.hDeviceWindow = hWnd;

		return m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dpp,
			&m_d3ddev);
	}

	void CLs3ThumbShlExt::CleanUpDirect3D()
	{
		m_d3ddev->Release();
		m_d3ddev = NULL;
		m_d3d->Release();
		m_d3d = NULL;
	}

	HRESULT CLs3ThumbShlExt::ReadImageFromDirect3D(HBITMAP *phBmpBitmap)
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
			hr = m_d3ddev->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &resolvedSurface, NULL);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = m_d3ddev->StretchRect(renderTarget, NULL, resolvedSurface, NULL, D3DTEXF_NONE);
			if (FAILED(hr))
			{
				return hr;
			}

			renderTarget = resolvedSurface;
		}

		IDirect3DSurface9 *offscreenSurface;
		hr = m_d3ddev->CreateOffscreenPlainSurface(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DPOOL_SYSTEMMEM, &offscreenSurface, NULL);
		if (FAILED(hr))
		{
			return hr;
		}

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

				*phBmpBitmap = hBmpThumbnail;
				
				offscreenSurface->UnlockRect();
				hr = S_OK;
			}
		}

		return hr;
	}

protected:
	// Full path to the file of which the thumbnail should be generated
	TCHAR m_szFilename[MAX_PATH];
	SIZE m_rgSize;
	LPDIRECT3D9 m_d3d;
	LPDIRECT3DDEVICE9 m_d3ddev;

	LPDIRECT3DVERTEXBUFFER9 m_vbuffer;
	LPDIRECT3DINDEXBUFFER9 m_ibuffer;

};

OBJECT_ENTRY_AUTO(__uuidof(Ls3ThumbShlExt), CLs3ThumbShlExt)
