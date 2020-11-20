#include "class_factory.hpp"

#include "shell_ext.hpp"

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <shlobj.h>
#include <windows.h>

ClassFactory::ClassFactory() {
  g_DllRefCount++;
}

ClassFactory::~ClassFactory() {
  g_DllRefCount--;
}

STDMETHODIMP ClassFactory::CreateInstance(LPUNKNOWN pUnknown, REFIID riid,
                                          LPVOID *ppObject) {
  *ppObject = NULL;
  if (pUnknown != NULL)
    return CLASS_E_NOAGGREGATION;

  try {
    boost::intrusive_ptr<Ls3ThumbShellExt> pShellExt(new Ls3ThumbShellExt(),
                                                     false);
    if (pShellExt == nullptr)
      return E_OUTOFMEMORY;
    return pShellExt->QueryInterface(riid, ppObject);
  } catch (const std::exception &e) {
    return E_FAIL;
  }
}
