// Ls3ThumbShlExt.h : Declaration of the CLs3ThumbShlExt

#pragma once

#include "resource.h"       // main symbols
#include "shobjidl.h"
#include "windows.h"
#include "windowsx.h"
#include "Thumbcache.h"

#include <memory>

#include "Ls3File.h"
#include "Ls3FileModificationDate.h"
#include "Ls3FileReader.h"
#include "LsFileReader.h"
#include "Ls3ThumbnailRenderer.h"

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
	public IExtractImage2,
	public IInitializeWithFile,
	public IThumbnailProvider
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
	COM_INTERFACE_ENTRY(IExtractImage2)
	COM_INTERFACE_ENTRY(IInitializeWithFile)
	COM_INTERFACE_ENTRY(IThumbnailProvider)
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
		wchar_t debug_buf[2048];
		wsprintf(debug_buf, L"Load() called for file %s\r\n", wszFile);
		OutputDebugString(debug_buf);

		USES_CONVERSION;
		lstrcpyn(m_szFilename, OLE2CT(wszFile), MAX_PATH);
		return S_OK;
	}

	// IExtractImage
	STDMETHOD(Extract)(HBITMAP* phBmpThumbnail)
	{
		wchar_t debug_buf[2048];
		wsprintf(debug_buf, L"Extract() called for file %s\r\n", m_szFilename);
		OutputDebugString(debug_buf);

		// Only show subsets visible in LOD1 when linked files are displayed.
		unique_ptr<Ls3File> ls3File;

		if (_wcsicmp(&m_szFilename[wcslen(m_szFilename) - 3], L".ls") == 0) {
			ls3File = LsFileReader::readLs3File(m_szFilename, 1000);
		}
		else {
			ls3File = Ls3FileReader::readLs3File(m_szFilename, 0x04);
		}

		if (ls3File->subsets.size() == 0) {
			wsprintf(debug_buf, L"File %s is empty, doing nothing\r\n",
				m_szFilename);
			OutputDebugString(debug_buf);
			return E_NOTIMPL;
		}

		Ls3ThumbnailRenderer renderer(_AtlBaseModule.GetModuleInstance());
		if (FAILED(renderer.RenderLs3File(*phBmpThumbnail, *ls3File, m_rgSize))) {
			return E_NOTIMPL;
		}
		else {
			return S_OK;
		}
	}

	STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
	{
		lstrcpyn(pszPathBuffer, m_szFilename, cchMax);

		// Save requested thumbnail size
		m_rgSize.cx = prgSize->cx;
		m_rgSize.cy = prgSize->cy;
		return S_OK;
	}

	// IExtractImage2
	STDMETHOD(GetDateStamp)(FILETIME *pDateStamp)
	{
		wchar_t debug_buf[2048];
		wsprintf(debug_buf, L"GetDateStamp() called for file %s\r\n", m_szFilename);
		OutputDebugString(debug_buf);

		Ls3FileModificationDate::GetLastModificationDate(m_szFilename,
			pDateStamp);
		return S_OK;
	}

	// IInitializeWithFile
	STDMETHOD(Initialize)(LPCWSTR pszFilePath, DWORD grfMode) {
		wchar_t debug_buf[2048];
		wsprintf(debug_buf, L"Ls3Thumb: Initialize() called for file %s\r\n", pszFilePath);
		OutputDebugString(debug_buf);

		lstrcpyn(m_szFilename, pszFilePath, MAX_PATH);
		return S_OK;
	}

	// IThumbnailProvider
	STDMETHOD(GetThumbnail)(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
	{
		wchar_t debug_buf[2048];
		wsprintf(debug_buf, L"Ls3Thumb: GetThumbnail() called for file %s\r\n", m_szFilename);
		OutputDebugString(debug_buf);

		m_rgSize = { cx, cx };
		return Extract(phbmp);
	}

protected:
	// Full path to the file of which the thumbnail should be generated
	TCHAR m_szFilename[MAX_PATH];
	SIZE m_rgSize;

};

OBJECT_ENTRY_AUTO(__uuidof(Ls3ThumbShlExt), CLs3ThumbShlExt)
