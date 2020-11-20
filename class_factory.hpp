#pragma once

#include "iunknown_impl.hpp"

extern HINSTANCE g_hInst;
extern UINT g_DllRefCount;

DEFINE_GetIIDForInterface(IClassFactory)

class ClassFactory : public IUnknownImpl<IClassFactory> {
 public:
  ClassFactory();
  virtual ~ClassFactory();

  // IClassFactory methods
  STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
  STDMETHODIMP LockServer(BOOL) {
    return E_NOTIMPL;
  };
};

inline void intrusive_ptr_add_ref(ClassFactory *obj) {
  obj->AddRef();
}
inline void intrusive_ptr_release(ClassFactory *obj) {
  obj->Release();
}
