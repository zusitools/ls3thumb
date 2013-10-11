// Ls3ThumbShlExt.h : Declaration of the CLs3ThumbShlExt

#pragma once
#include "resource.h"       // main symbols
#include "shobjidl.h"
#include "windows.h"
#include "windowsx.h"
#include "d3d9.h"

#include "Ls3Thumb_i.h"



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
		COLORREF resultColor = RGB(255, 0, 0);

		// Create a window class for our hidden window
		WNDCLASSEX wc;
		wc.hInstance = _AtlBaseModule.GetModuleInstance();
		wc.lpszClassName = (LPCWSTR) L"Ls3ThumbShlExtHiddenWindow";
		wc.lpfnWndProc = &CLs3ThumbShlExt::DummyWndProc;
		wc.style = CS_DBLCLKS;
		wc.cbSize = sizeof (WNDCLASSEX);
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszMenuName = NULL;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

		if (RegisterClassEx(&wc))
		{
			resultColor = RGB(0, 0, 255);
		}

		HWND hwnd = CreateWindowEx(0,
			(LPCWSTR) L"Ls3ThumbShlExtHiddenWindow",
			(LPCWSTR) L"Hidden Window",
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			400, 300,
			NULL,
			NULL,
			_AtlBaseModule.GetModuleInstance(),
			NULL);

		if (hwnd)
		{
			resultColor = RGB(255, 0, 255);
		}

		if (SUCCEEDED(this->InitDirect3D(hwnd)))
		{
			resultColor = RGB(128, 128, 128);

			m_d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 45, 100), 1.0f, 0);
			m_d3ddev->BeginScene();
			m_d3ddev->EndScene();
			m_d3ddev->Present(NULL, NULL, NULL, NULL);

			this->CleanUpDirect3D();
		}

		DestroyWindow(hwnd);

		// Create a dummy image, just for testing
		BITMAPINFO bmInfo;
		memset(&bmInfo.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
		bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth = m_rgSize.cx;
		bmInfo.bmiHeader.biHeight = m_rgSize.cy;
		bmInfo.bmiHeader.biPlanes = 1;
		bmInfo.bmiHeader.biBitCount = 32;

		HDC pDC = GetDC(NULL);
		HDC tmpDC = CreateCompatibleDC(pDC);
		UINT32 *pPixels;
		HBITMAP hBmpThumbnail = CreateDIBSection(tmpDC, &bmInfo,
			DIB_RGB_COLORS, (void**) &pPixels, 0, 0);

		// Fill image with the result color
		for (int i = bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biHeight - 1;
			i >= 0; i--)
		{
			pPixels[i] = resultColor;
		}

		*phBmpThumbnail = hBmpThumbnail;
		return S_OK;
	}

	STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
	{
		// Save requested thumbnail size
		m_rgSize.cx = prgSize->cx;
		m_rgSize.cy = prgSize->cy;
		return S_OK;
	}

	// Dummy Window
	static LRESULT CALLBACK DummyWndProc(HWND hWnd, UINT msg,
		WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
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

protected:
	// Full path to the file of which the thumbnail should be generated
	TCHAR m_szFilename[MAX_PATH];
	SIZE m_rgSize;
	LPDIRECT3D9 m_d3d;
	LPDIRECT3DDEVICE9 m_d3ddev;

};

OBJECT_ENTRY_AUTO(__uuidof(Ls3ThumbShlExt), CLs3ThumbShlExt)
