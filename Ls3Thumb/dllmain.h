// dllmain.h : Declaration of module class.

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

class CLs3ThumbModule : public ATL::CAtlDllModuleT< CLs3ThumbModule >
{
public :
	DECLARE_LIBID(LIBID_Ls3ThumbLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LS3THUMB, "{A7FE12BD-7730-431F-A79A-0F074582FFFF}")
};

extern class CLs3ThumbModule _AtlModule;
