#include "class_factory.hpp"
#include "shell_ext.hpp"

#define INITGUID
#include "guid.hpp"
#include <initguid.h>

#include "ls3render/window.hpp"

#include <boost/nowide/stackstring.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <olectl.h>
#include <shlobj.h>
#include <strsafe.h>
#include <windows.h>

#include <optional>
#include <variant>

// data
HINSTANCE g_hInst;
UINT g_DllRefCount;
DWORD g_mainThreadId;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call,
                      LPVOID /*lpReserved*/) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      g_hInst = (HINSTANCE)hModule;
      g_mainThreadId = GetCurrentThreadId();
      DisableThreadLibraryCalls(reinterpret_cast<HMODULE>(hModule));
      ls3render::OpenGlWindow::RegisterWindowClass(g_hInst);
      break;
    default:
      break;
  }
  return TRUE;
}

/*---------------------------------------------------------------*/
// DllCanUnloadNow()
/*---------------------------------------------------------------*/
STDAPI DllCanUnloadNow(VOID) {
  return (g_DllRefCount ? S_FALSE : S_OK);
}

/*---------------------------------------------------------------*/
// DllGetClassObject()
/*---------------------------------------------------------------*/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppReturn) {
  *ppReturn = NULL;

  if (!IsEqualCLSID(rclsid, CLSID_Ls3ThumbShlExt)) {
    return CLASS_E_CLASSNOTAVAILABLE;
  }

  boost::intrusive_ptr<ClassFactory> pClassFactory(new ClassFactory(), false);
  if (pClassFactory == NULL) {
    return E_OUTOFMEMORY;
  }

  return pClassFactory->QueryInterface(riid, ppReturn);
}

/*---------------------------------------------------------------*/
// DllGetRegisterServer()
/*---------------------------------------------------------------*/

struct RegistryEntry {
  HKEY hRootKey;
  std::string subKey;
  std::optional<std::string> valueName;
  std::variant<std::string, DWORD> data;
};

STDAPI DllRegisterServer(VOID) {
  const std::string dllPath = []() {
    TCHAR buf[MAX_PATH];
    GetModuleFileName(g_hInst, buf, ARRAYSIZE(buf));
    return boost::nowide::narrow(buf);
  }();

  using namespace std::literals;

  const RegistryEntry registryEntries[] = {
    // clang-format off
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, {}, "Ls3Thumb"s },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, "InfoTip"s, "Ls3Thumb."s },

    // necessary for IInitializeWithFile implementation which we need because we need the file path to load linked files, textures, and lsb files.
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, "DisableProcessIsolation", 1u },

#if _WIN64
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, "AppId", "{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}" },  // prevhost.exe surrogate host
#else
    // 32-bit preview handlers should use AppID {534A1E02-D58F-44f0-B58B-36CBED287C7C} when installed on 64-bit operating systems.
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, "AppId", "{534A1E02-D58F-44f0-B58B-36CBED287C7C}" },
#endif

    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\InprocServer32", {}, dllPath },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\InprocServer32", "ThreadingModel", "Apartment" },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\InprocServer32", "ProgID", "Zusi3DEditorls3" },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\DefaultIcon", {}, dllPath + ",0" },

    // Thumbnail handler
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{e357fccd-a995-4576-b01f-234630154e96}", {}, STRCLSID_Ls3ThumbShlExt },
    { HKEY_CLASSES_ROOT, "xmlfile\\shellex\\{e357fccd-a995-4576-b01f-234630154e96}", {}, STRCLSID_Ls3ThumbShlExt },
    
    // IExtractImage
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}", {}, STRCLSID_Ls3ThumbShlExt },
    { HKEY_CLASSES_ROOT, "xmlfile\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}", {}, STRCLSID_Ls3ThumbShlExt },
    
    // Preview handler
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}", {}, STRCLSID_Ls3ThumbShlExt },
    { HKEY_CLASSES_ROOT, "xmlfile\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}", {}, STRCLSID_Ls3ThumbShlExt },
    { HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", STRCLSID_Ls3ThumbShlExt, "Ls3Thumb"},

    { HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", STRCLSID_Ls3ThumbShlExt, "Ls3Thumb" }
    // clang-format on
  };

  for (const auto &entry : registryEntries) {
    HKEY hKey;
    DWORD dwDisp;
    LRESULT lResult = RegCreateKeyEx(
        entry.hRootKey, boost::nowide::widen(entry.subKey).c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult == NOERROR) {
      std::visit(
          [&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
              const auto dataWide = boost::nowide::widen(arg);
              lResult = RegSetValueEx(
                  hKey,
                  entry.valueName.has_value()
                      ? boost::nowide::widen(*entry.valueName).c_str()
                      : nullptr,
                  0, REG_SZ, reinterpret_cast<const BYTE *>(dataWide.data()),
                  (dataWide.size() + 1 /* null terminator */) *
                      sizeof(typename decltype(dataWide)::value_type));
            } else {
              static_assert(std::is_same_v<T, DWORD>);
              lResult = RegSetValueEx(
                  hKey,
                  entry.valueName.has_value()
                      ? boost::nowide::widen(*entry.valueName).c_str()
                      : nullptr,
                  0, REG_DWORD, (LPBYTE)&arg, sizeof arg);
            }
          },
          entry.data);
      RegCloseKey(hKey);
    } else {
      return SELFREG_E_CLASS;
    }
  }

  // Invalidate the thumbnail cache.
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

  return S_OK;
}

/*---------------------------------------------------------------*/
// DllUnregisterServer()
/*---------------------------------------------------------------*/
STDAPI DllUnregisterServer(VOID) {
  using namespace std::literals;

  const RegistryEntry registryEntries[] = {
      // clang-format off
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt, {}, {} },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\InprocServer32", {}, {} },
    { HKEY_CLASSES_ROOT, "CLSID\\"s + STRCLSID_Ls3ThumbShlExt + "\\DefaultIcon", {}, {} },
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{e357fccd-a995-4576-b01f-234630154e96}", {}, {} },
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{953BB1EE-93B4-11d1-98A3-00C04FB687DA}", {}, {} },
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}", {}, {} },
    { HKEY_CLASSES_ROOT, "Zusi3DEditorls3\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}", {}, {} },
    { HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", STRCLSID_Ls3ThumbShlExt, {} },
    { HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", STRCLSID_Ls3ThumbShlExt, {} }
      // clang-format on
  };

  for (const auto &entry : registryEntries) {
    if (entry.valueName.has_value()) {
      HKEY hKey;
      LRESULT lResult = RegOpenKeyEx(entry.hRootKey,
                                     boost::nowide::widen(entry.subKey).c_str(),
                                     0, KEY_SET_VALUE, &hKey);
      if (lResult == ERROR_SUCCESS) {
        lResult = RegDeleteValue(
            hKey, boost::nowide::widen(*entry.valueName).c_str());
        RegCloseKey(hKey);
      }
    } else {
      RegDeleteKey(entry.hRootKey, boost::nowide::widen(entry.subKey).c_str());
    }
  }

  return S_OK;
}
