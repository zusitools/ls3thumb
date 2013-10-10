// dllmain.h : Declaration of module class.

class CLs3ThumbModule : public ATL::CAtlDllModuleT< CLs3ThumbModule >
{
public :
	DECLARE_LIBID(LIBID_Ls3ThumbLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LS3THUMB, "{A7FE12BD-7730-431F-A79A-0F074582FFFF}")
};

extern class CLs3ThumbModule _AtlModule;
