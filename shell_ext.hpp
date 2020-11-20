#pragma once

#include "thumbcache.h"  // IThumbnailProvider
#include <shobjidl.h>

#include "shader.hpp"

#include "iunknown_impl.hpp"

#include <glm/glm.hpp>

#include <atomic>
#include <future>
#include <optional>
#include <string>

extern HINSTANCE g_hInst;

namespace ls3render {
class Scene;
class OpenGlWindow;
}  // namespace ls3render

DEFINE_GetIIDForInterface(IExtractImage)
DEFINE_GetIIDForInterface(IInitializeWithFile)
DEFINE_GetIIDForInterface(IObjectWithSite)
DEFINE_GetIIDForInterface(IOleWindow)
DEFINE_GetIIDForInterface(IPersistFile)
DEFINE_GetIIDForInterface(IPreviewHandler)
DEFINE_GetIIDForInterface(IPreviewHandlerVisuals)
DEFINE_GetIIDForInterface(IThumbnailProvider)

class Ls3ThumbShellExt
    : public IUnknownImpl<IPersistFile, IExtractImage, IInitializeWithFile,
                          IThumbnailProvider, IObjectWithSite, IPreviewHandler,
                          IPreviewHandlerVisuals, IOleWindow> {
 public:
  Ls3ThumbShellExt();
  ~Ls3ThumbShellExt();

  // IPersistFile
  HRESULT STDMETHODCALLTYPE GetClassID(CLSID *) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE IsDirty() override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE Save(LPCOLESTR, BOOL) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *) override {
    return E_NOTIMPL;
  }

  HRESULT STDMETHODCALLTYPE Load(LPCOLESTR wszFile, DWORD /*dwMode*/);

  // IExtractImage
  HRESULT STDMETHODCALLTYPE Extract(HBITMAP *phBmpThumbnail) override;
  HRESULT STDMETHODCALLTYPE GetLocation(LPWSTR pszPathBuffer, DWORD cchMax,
                                        DWORD *pdwPriority, const SIZE *prgSize,
                                        DWORD dwRecClrDepth,
                                        DWORD *pdwFlags) override;

  // IInitializeWithFile
  HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR pszFilePath,
                                       DWORD grfMode) override;

  // IThumbnailProvider
  HRESULT STDMETHODCALLTYPE GetThumbnail(UINT cx, HBITMAP *phbmp,
                                         WTS_ALPHATYPE *pdwAlpha) override;

  // IObjectWithSite
  HRESULT STDMETHODCALLTYPE SetSite(IUnknown *punkSite) override;
  HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppv) override;

  // IPreviewHandler
  HRESULT STDMETHODCALLTYPE SetWindow(HWND hwnd, const RECT *prc) override;
  HRESULT STDMETHODCALLTYPE SetFocus() override;
  HRESULT STDMETHODCALLTYPE QueryFocus(HWND *phwnd) override;
  HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG *pmsg) override;
  HRESULT STDMETHODCALLTYPE SetRect(const RECT *prc) override;
  HRESULT STDMETHODCALLTYPE DoPreview() override;
  HRESULT STDMETHODCALLTYPE Unload() override;

  // IPreviewHandlerVisuals
  HRESULT STDMETHODCALLTYPE SetBackgroundColor(COLORREF color) override;
  HRESULT STDMETHODCALLTYPE SetFont(const LOGFONTW *) override {
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE SetTextColor(COLORREF) override {
    return S_OK;
  }

  // IOleWindow
  HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd) override;
  HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override {
    return E_NOTIMPL;
  }

  LRESULT CALLBACK static WindowProcedure(HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam);

 private:
  HRESULT _CreateGlfwWindow();
  HRESULT _CreatePreviewWindow();

  // For both thumbnail and preview handler: Full path to the file of which the
  // thumbnail/preview should be generated, and size of the thumbnail/preview
  // window.
  std::string _fileName;
  std::pair<size_t, size_t> _previewSize;

  // GLFW window that also holds the OpenGL context.
  // It is invisible for thumbnail generation, and visible for preview
  // generation.
  std::unique_ptr<ls3render::OpenGlWindow> _window;

  // Rest is for preview handler.
  HWND _hwndParent{};     // parent window of the previewer window.
  IUnknown *_punkSite{};  // site pointer from host, used for IObjectWithSite
                          // implementation.

  std::thread _sceneLoadThread{};

  std::mutex _sceneAndBboxMutex{};
  std::unique_ptr<ls3render::Scene> _scene{nullptr};
  std::pair<glm::vec3, glm::vec3> _bbox;

  std::atomic<bool> _cancelSceneLoading{};
  bool _sceneLoaded{false};
  COLORREF _backgroundColor{0x00FFFFFF};  // default to white

  std::optional<std::pair<int, int>> _lastCursorPos{};
  std::optional<float> _rotationZ;
  std::optional<float> _rotationX;
  bool _mouseButtonPressed{false};

 public:
  WNDPROC _prevWndProc;
};

inline void intrusive_ptr_add_ref(Ls3ThumbShellExt *obj) {
  obj->AddRef();
}
inline void intrusive_ptr_release(Ls3ThumbShellExt *obj) {
  obj->Release();
}
