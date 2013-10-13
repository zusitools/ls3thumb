// Ls3ThumbShlExt.h : Declaration of the CLs3ThumbShlExt

#pragma once

#include "resource.h"       // main symbols
#include "shobjidl.h"
#include "windows.h"
#include "windowsx.h"

#include <memory>

#include "Ls3File.h"
#include "Ls3FileReader.h"
#include "Ls3FileRenderer.h"

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
		char fileName[MAX_PATH];
		size_t i;
		wcstombs_s(&i, fileName, MAX_PATH, m_szFilename, MAX_PATH);
		
		std::unique_ptr<Ls3File> ls3File(Ls3FileReader::readLs3File(fileName));

		if (ls3File->subsets.size() == 0) {
			return S_OK;
		}

		Ls3FileRenderer renderer(_AtlBaseModule.GetModuleInstance());
		return renderer.RenderLs3File(*phBmpThumbnail, *ls3File, m_rgSize);
	}

	STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
	{
		// Save requested thumbnail size
		m_rgSize.cx = prgSize->cx;
		m_rgSize.cy = prgSize->cy;
		return S_OK;
	}

protected:
	// Full path to the file of which the thumbnail should be generated
	TCHAR m_szFilename[MAX_PATH];
	SIZE m_rgSize;

};

OBJECT_ENTRY_AUTO(__uuidof(Ls3ThumbShlExt), CLs3ThumbShlExt)
