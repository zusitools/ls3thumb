#include "shell_ext.hpp"

#include "debug.hpp"
#include "guid.hpp"

#include "./ls3render/macros.hpp"
#include "./ls3render/window.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "shader_parameters.hpp"

#include <GL/wglew.h>  // needs to be included before gl.h

#include <GL/gl.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "thumbcache.h"  // for IID_IThumbnailProvider
#include <initguid.h>
#include <shlobj.h>
#include <tchar.h>
#include <windows.h>

#include <future>
#include <string>

extern DWORD g_mainThreadId;

namespace {
static const std::unordered_map<int, float> AniPositionen{
    {6, 0.5f},   // Gleiskruemmung
    {7, 0.5f},   // Gleiskruemmung
    {14, 0.5f},  // Neigetechnik
};
static const std::string_view lodLs3Ext(".lod.ls3");

template <typename T>
struct scope_exit {
  scope_exit(T &&t) : t_{std::move(t)} {
  }
  ~scope_exit() {
    t_();
  }
  T t_;
};

HRESULT RenderScene(const std::pair<size_t, size_t> &previewSize,
                    ls3render::Scene *scene, bool &sceneLoaded,
                    const std::pair<glm::vec3, glm::vec3> &bbox,
                    COLORREF backgroundColor, std::optional<float> rotationX,
                    std::optional<float> rotationZ) {
  const auto viewportWidth = previewSize.first;
  const auto viewportHeight = previewSize.second;
  TRY(glViewport(0, 0, viewportWidth, viewportHeight));

  TRY(glClearColor(GetRValue(backgroundColor) / 255.0f,
                   GetGValue(backgroundColor) / 255.0f,
                   GetBValue(backgroundColor) / 255.0f, 0.0f));
  TRY(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  if (scene == nullptr) {
    return S_OK;
  }

  if (!sceneLoaded) {
    // Load data into graphics card memory
    if (!scene->LoadIntoGraphicsCardMemory()) {
      DEBUG("Loading data into graphics card memory failed");
      return E_FAIL;
    }
    sceneLoaded = true;
  }

  // Enable depth test
  TRY(glEnable(GL_DEPTH_TEST));

  // Enable backface culling
  TRY(glEnable(GL_CULL_FACE));
  TRY(glFrontFace(GL_CW));

  // Enable depth offset
  TRY(glEnable(GL_POLYGON_OFFSET_FILL));

  ls3render::ShaderManager shaderManager;
  const auto &shaderParameters = shaderManager.getShaderParameters();

  constexpr const float fovY = 3.141592 / 4;

  const float aspectRatio = (float)viewportWidth / (float)viewportHeight;
  const float fovX = 2 * atan(tan(fovY * 0.5f) * aspectRatio);

  const glm::vec3 bboxCornerToCorner{bbox.second.x - bbox.first.x,
                                     bbox.second.y - bbox.first.y,
                                     bbox.second.z - bbox.first.z};

  // Camera always looks to the center of the bounding box.
  glm::vec3 cameraTarget((bbox.first.x + bbox.second.x) / 2,
                         (bbox.first.y + bbox.second.y) / 2,
                         (bbox.first.z + bbox.second.z) / 2);

  // Position the camera so that the bounding sphere is visible.
  const float boundingSphereRadius = glm::length(bboxCornerToCorner) / 2;
  const float cameraDistance =
      boundingSphereRadius / std::min(sin(fovY / 2.0f), sin(fovX / 2.0f));
  const glm::vec3 cameraOffset(glm::rotateZ(
      glm::rotateY(glm::vec3{cameraDistance, 0, 0}, *rotationX), *rotationZ));
  const glm::vec3 cameraUp = glm::rotateZ(
      glm::rotateY(glm::vec3{0.0f, 0.0f, 1.0f}, *rotationX), *rotationZ);

  const glm::mat4 view =
      glm::lookAt(cameraTarget + cameraOffset, cameraTarget, cameraUp);
  TRY(glUniformMatrix4fv(shaderParameters.uni_view, 1, GL_FALSE,
                         glm::value_ptr(view)));

  const glm::mat4 proj =
      glm::perspective(fovY, aspectRatio, cameraDistance - boundingSphereRadius,
                       cameraDistance + boundingSphereRadius);
  TRY(glUniformMatrix4fv(shaderParameters.uni_proj, 1, GL_FALSE,
                         glm::value_ptr(proj)));

  scene->Render(shaderParameters);

  return S_OK;
}

}  // namespace

Ls3ThumbShellExt::Ls3ThumbShellExt() = default;

Ls3ThumbShellExt::~Ls3ThumbShellExt() {
  _cancelSceneLoading = true;
  if (_sceneLoadThread.joinable()) {
    _sceneLoadThread.join();
  }
  {
    std::unique_lock lock(_sceneAndBboxMutex);
    if (_scene) {
      if (_sceneLoaded) {
        assert(_window);
        auto context = _window->GetContext();
        _scene->FreeGraphicsCardMemory();  // needs a context
        _sceneLoaded = false;
      }
    }
  }
}

STDMETHODIMP Ls3ThumbShellExt::Load(LPCOLESTR wszFile, DWORD /*dwMode*/) {
  _fileName = boost::nowide::narrow(wszFile);
  return S_OK;
}

// IExtractImage
STDMETHODIMP Ls3ThumbShellExt::Extract(HBITMAP *phBmpThumbnail) {
  try {
    if (_window == nullptr) {
      if (!SUCCEEDED(_CreateGlfwWindow())) {
        return E_FAIL;
      }
    }

    auto context =
        _window->GetContext();  // must be allocated before "scene",
                                // as scene cleanup requires a context

    // Enable depth test
    TRY(glEnable(GL_DEPTH_TEST));

    // Enable backface culling
    TRY(glEnable(GL_CULL_FACE));
    TRY(glFrontFace(GL_CW));

    // Enable depth offset
    TRY(glEnable(GL_POLYGON_OFFSET_FILL));

    const auto scene = std::make_unique<ls3render::Scene>();
    std::atomic<bool> cancelSceneLoading{false};
    if (!scene->LadeLandschaft(zusixml::ZusiPfad::vonOsPfad(_fileName),
                               glm::mat4{1}, AniPositionen,
                               cancelSceneLoading)) {
      DEBUG("Loading scenery failed.");
      return E_FAIL;
    }
    const auto bbox = scene->GetBoundingBox();

    const glm::vec2 bboxCornerToCorner{bbox.second.x - bbox.first.x,
                                       bbox.first.y - bbox.second.y};
    const auto rotationZ = glm::orientedAngle(
        glm::normalize(bboxCornerToCorner), glm::vec2{0, -1});
    const auto rotationX = -0.3;  // view slightly from above

    BITMAPINFO bmInfo = {};
    bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth = _previewSize.first;
    bmInfo.bmiHeader.biHeight = _previewSize.second;
    bmInfo.bmiHeader.biSizeImage = _previewSize.first * _previewSize.second * 4;
    bmInfo.bmiHeader.biPlanes = 1;
    bmInfo.bmiHeader.biBitCount = 32;
    bmInfo.bmiHeader.biCompression = BI_RGB;

    HDC pDC = GetDC(nullptr);
    HDC tmpDC = CreateCompatibleDC(pDC);
    void *pPixels;
    *phBmpThumbnail =
        CreateDIBSection(tmpDC, &bmInfo, DIB_RGB_COLORS, &pPixels, 0, 0);
    // "You need to guarantee that the GDI subsystem has completed any drawing
    // to a bitmap created by CreateDIBSection before you draw to the bitmap
    // yourself. Access to the bitmap must be synchronized. Do this by calling
    // the GdiFlush function."
    GdiFlush();

    if (*phBmpThumbnail == nullptr) {
      DEBUG("CreateDIBSection failed");
      return E_FAIL;
    }

    // Create and bind framebuffer & renderbuffer
    GLuint framebuffer;
    TRY(glGenFramebuffers(1, &framebuffer));
    TRY(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));

    GLuint color_renderbuffer;
    TRY(glGenRenderbuffers(1, &color_renderbuffer));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer));

    TRY(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _previewSize.first,
                              _previewSize.second));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER,
                           0));  // don't know why that is necessary

    TRY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, color_renderbuffer));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      DEBUG("Non-multisampling framebuffer is not complete");
      return E_FAIL;
    }

    // Create and bind multisampling framebuffer & renderbuffers
    GLuint framebuffer_multisample;
    TRY(glGenFramebuffers(1, &framebuffer_multisample));
    TRY(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_multisample));

    GLuint color_renderbuffer_multisample;
    TRY(glGenRenderbuffers(1, &color_renderbuffer_multisample));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer_multisample));

    const int multisampling = 0;
    TRY(glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisampling,
                                         GL_RGBA, _previewSize.first,
                                         _previewSize.second));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER,
                           0));  // don't know why that is necessary

    TRY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER,
                                  color_renderbuffer_multisample));

    GLuint depth_renderbuffer_multisample;
    TRY(glGenRenderbuffers(1, &depth_renderbuffer_multisample));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_multisample));

    TRY(glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisampling,
                                         GL_DEPTH_COMPONENT, _previewSize.first,
                                         _previewSize.second));
    TRY(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    TRY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  depth_renderbuffer_multisample));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      DEBUG("Multisampling framebuffer is not complete");
      return E_FAIL;
    }

    bool sceneLoaded = false;
    if (!SUCCEEDED(RenderScene(_previewSize, scene.get(), sceneLoaded, bbox,
                               0xffffff, rotationX, rotationZ))) {
      DEBUG("RenderScene failed");
      return E_FAIL;
    }

    TRY(glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_multisample));
    TRY(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer));
    TRY(glBlitFramebuffer(0, 0, _previewSize.first, _previewSize.second, 0, 0,
                          _previewSize.first, _previewSize.second,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR));

    TRY(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));
    TRY(glReadBuffer(GL_COLOR_ATTACHMENT0));
    TRY(glReadPixels(0, 0, _previewSize.first, _previewSize.second, GL_BGRA_EXT,
                     GL_UNSIGNED_BYTE, pPixels));

    TRY(glDeleteFramebuffers(1, &framebuffer));
    TRY(glDeleteRenderbuffers(1, &color_renderbuffer));
    TRY(glDeleteFramebuffers(1, &framebuffer_multisample));
    TRY(glDeleteRenderbuffers(1, &color_renderbuffer_multisample));
    TRY(glDeleteRenderbuffers(1, &depth_renderbuffer_multisample));

    return S_OK;
  } catch (const std::exception &e) {
    DEBUG(e.what());
    return E_FAIL;
  }
}

bool IsLodLs3(const std::string fileName) {
  return ((fileName.size() >= lodLs3Ext.size()) &&
          (0 == _strnicmp(&fileName[fileName.size() - lodLs3Ext.size()],
                          lodLs3Ext.data(), lodLs3Ext.size())));
}

STDMETHODIMP Ls3ThumbShellExt::GetLocation(LPWSTR pszPathBuffer, DWORD cchMax,
                                           DWORD * /*pdwPriority*/,
                                           const SIZE *prgSize,
                                           DWORD /*dwRecClrDepth*/,
                                           DWORD * /*pdwFlags*/) {
  if (!IsLodLs3(_fileName)) {
    return E_NOTIMPL;
  }

  boost::nowide::widen(pszPathBuffer, cchMax, &*_fileName.begin(),
                       &*_fileName.end());

  // Save requested thumbnail size
  _previewSize = {prgSize->cx, prgSize->cy};
  return S_OK;
}

STDMETHODIMP Ls3ThumbShellExt::Initialize(LPCWSTR pszFilePath,
                                          DWORD /*grfMode*/) {
  _fileName = boost::nowide::narrow(pszFilePath);
  return S_OK;
}

STDMETHODIMP Ls3ThumbShellExt::GetThumbnail(UINT cx, HBITMAP *phbmp,
                                            WTS_ALPHATYPE *pdwAlpha) {
  if (!IsLodLs3(_fileName)) {
    return E_NOTIMPL;
  }

  _previewSize = {static_cast<LONG>(cx), static_cast<LONG>(cx)};
  *pdwAlpha = WTSAT_ARGB;
  return Extract(phbmp);
}

// IPreviewHandler
// This method gets called when the previewer gets created
HRESULT Ls3ThumbShellExt::SetWindow(HWND hwnd, const RECT *prc) {
  if (hwnd && prc) {
    _hwndParent = hwnd;  // cache the HWND for later use
    _previewSize = {prc->right - prc->left, prc->bottom - prc->top};

    if (_window) {
      // Update preview window parent and rect information
      _window->SetParent(_hwndParent);
      _window->SetPositionAndSize({prc->left, prc->top}, _previewSize);
    }
  }
  return S_OK;
}

HRESULT Ls3ThumbShellExt::SetFocus() {
  HRESULT hr = S_FALSE;
  if (_window) {
    _window->Focus();
    hr = S_OK;
  }
  return hr;
}

HRESULT Ls3ThumbShellExt::QueryFocus(HWND *phwnd) {
  HRESULT hr = E_INVALIDARG;
  if (phwnd) {
    *phwnd = ::GetFocus();
    if (*phwnd) {
      hr = S_OK;
    } else {
      hr = HRESULT_FROM_WIN32(GetLastError());
    }
  }
  return hr;
}

HRESULT Ls3ThumbShellExt::TranslateAccelerator(MSG *pmsg) {
  HRESULT hr = S_FALSE;
  IPreviewHandlerFrame *pFrame = nullptr;
  if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(&pFrame))) {
    // If your previewer has multiple tab stops, you will need to do appropriate
    // first/last child checking. This particular sample previewer has no
    // tabstops, so we want to just forward .this message out
    hr = pFrame->TranslateAccelerator(pmsg);
    if (pFrame != nullptr) {
      pFrame->Release();
      pFrame = nullptr;
    }
  }
  return hr;
}

// This method gets called when the size of the previewer window changes (user
// resizes the Reading Pane)
HRESULT Ls3ThumbShellExt::SetRect(const RECT *prc) {
  HRESULT hr = E_INVALIDARG;
  if (prc) {
    _previewSize = {prc->right - prc->left, prc->bottom - prc->top};
    if (_window) {
      // Preview window is already created, so set its size and position
      _window->SetPositionAndSize({prc->left, prc->top}, _previewSize);
      // will trigger WM_PAINT -> redraw
    } else {
      _CreatePreviewWindow();
      if (_window == nullptr) {
        return E_FAIL;
      }

      {
        std::unique_lock lock(_sceneAndBboxMutex);
        auto context = _window->GetContext();
        if (!SUCCEEDED(RenderScene(_previewSize, _scene.get(), _sceneLoaded,
                                   _bbox, _backgroundColor, *_rotationX,
                                   _rotationZ))) {
          return E_FAIL;
        }
        TRY(_window->SwapBuffers());
      }
    }
    hr = S_OK;
  }

  return hr;
}

// The main method that extracts relevant information from the file stream and
// draws content to the previewer window
HRESULT Ls3ThumbShellExt::DoPreview() {
  try {
    if (_scene == nullptr) {
      _cancelSceneLoading = false;
      if (_sceneLoadThread.joinable()) {
        _sceneLoadThread.join();
      }
      _sceneLoadThread = std::thread([this]() {
        auto scene = std::make_unique<ls3render::Scene>();
        if (!scene->LadeLandschaft(zusixml::ZusiPfad::vonOsPfad(_fileName),
                                   glm::mat4{1}, AniPositionen,
                                   _cancelSceneLoading)) {
          DEBUG("Loading scenery failed.");
          return;
        }

        {
          std::unique_lock lock(_sceneAndBboxMutex);
          _bbox = scene->GetBoundingBox();
          _scene = std::move(scene);

          const glm::vec2 bboxCornerToCorner{_bbox.second.x - _bbox.first.x,
                                             _bbox.first.y - _bbox.second.y};
          if (!_rotationZ.has_value()) {
            _rotationZ = glm::orientedAngle(glm::normalize(bboxCornerToCorner),
                                            glm::vec2{0, -1});
          }
          if (!_rotationX.has_value()) {
            _rotationX = -0.3;  // view slightly from above
          }
        }

        if (_window != nullptr) {
          _window->Repaint();
        }
      });
    }

    if ((_previewSize.first == 0 || _previewSize.first == 0)) {
      return S_OK;  // rect not yet set, _CreatePreviewWindow would fail
    }

    if (_window == nullptr) {
      if (FAILED(_CreatePreviewWindow())) {
        DEBUG("Creating the preview window failed");
        return E_FAIL;
      }
    } else {
      _window->Show();
    }

    {
      std::unique_lock lock(_sceneAndBboxMutex);
      auto context = _window->GetContext();
      if (!SUCCEEDED(RenderScene(_previewSize, _scene.get(), _sceneLoaded,
                                 _bbox, _backgroundColor, _rotationX,
                                 _rotationZ))) {
        return E_FAIL;
      }
      TRY(_window->SwapBuffers());
    }

    return S_OK;
  } catch (const std::exception &e) {
    DEBUG(e.what());
    return E_FAIL;
  }
}

HRESULT Ls3ThumbShellExt::_CreateGlfwWindow() {
  try {
    _window = std::make_unique<ls3render::OpenGlWindow>(g_hInst, _previewSize);
  } catch (const std::runtime_error &e) {
    DEBUG(e.what());
    return E_FAIL;
  }

  return S_OK;
}

HRESULT Ls3ThumbShellExt::_CreatePreviewWindow() {
  try {
    if (!SUCCEEDED(_CreateGlfwWindow())) {
      return E_FAIL;
    }

    _window->SetParent(_hwndParent);
    _window->SetOnRefresh([this] {
      {
        std::unique_lock lock(_sceneAndBboxMutex);
        auto context = _window->GetContext();
        RenderScene(_previewSize, _scene.get(), _sceneLoaded, _bbox,
                    _backgroundColor, _rotationX, _rotationZ);
        TRY(_window->SwapBuffers());
      }
    });
    _window->SetOnMouseMove([this](const std::pair<int, int> &pos) {
      if (_lastCursorPos.has_value() && _mouseButtonPressed) {
        if (_rotationZ.has_value()) {
          *_rotationZ += 3.14 * ((pos.first - _lastCursorPos->first) /
                                 static_cast<float>(_previewSize.first));
        }
        if (_rotationX.has_value()) {
          *_rotationX += 3.14 * ((pos.second - _lastCursorPos->second) /
                                 static_cast<float>(_previewSize.second));
        }
        {
          std::unique_lock lock(_sceneAndBboxMutex);
          auto context = _window->GetContext();
          RenderScene(_previewSize, _scene.get(), _sceneLoaded, _bbox,
                      _backgroundColor, *_rotationX, *_rotationZ);
          TRY(_window->SwapBuffers());
        }
      }
      _lastCursorPos = pos;
    });
    _window->SetOnLeftMouseButton(
        [this](const std::pair<int, int> &pos, bool down) {
          _mouseButtonPressed = down;
          _lastCursorPos = pos;
        });
    _window->SetOnKeyDown([this](int key) {
      if (key == VK_RIGHT) {
        if (_rotationZ.has_value()) {
          *_rotationZ += 0.1;
        }
      } else if (key == VK_LEFT) {
        if (_rotationZ.has_value()) {
          *_rotationZ -= 0.1;
        }
      } else if (key == VK_UP) {
        if (_rotationX.has_value()) {
          *_rotationX += 0.1;
        }
      } else if (key == VK_DOWN) {
        if (_rotationX.has_value()) {
          *_rotationX -= 0.1;
        }
      } else {
        return;
      }
      {
        std::unique_lock lock(_sceneAndBboxMutex);
        auto context = _window->GetContext();
        RenderScene(_previewSize, _scene.get(), _sceneLoaded, _bbox,
                    _backgroundColor, _rotationX, _rotationZ);
        TRY(_window->SwapBuffers());
      }
    });

    _window->Show();
    return S_OK;
  } catch (const std::exception &e) {
    DEBUG(e.what());
    return E_FAIL;
  }
}

// This method gets called when a shell item is de-selected in the listview
HRESULT Ls3ThumbShellExt::Unload() {
  _cancelSceneLoading = true;
  if (_sceneLoadThread.joinable()) {
    _sceneLoadThread.join();
  }
  {
    std::unique_lock lock(_sceneAndBboxMutex);
    if (_scene) {
      if (_sceneLoaded) {
        assert(_window);
        auto context = _window->GetContext();
        _scene->FreeGraphicsCardMemory();  // needs a context
        _sceneLoaded = false;
      }
      _scene = nullptr;
    }
  }
  if (_window) {
    _window->Hide();
    _window->SetParent(nullptr);
  }
  return S_OK;
}

// IPreviewHandlerVisuals methods
HRESULT Ls3ThumbShellExt::SetBackgroundColor(COLORREF color) {
  _backgroundColor = color;
  return S_OK;
}

// IObjectWithSite methods
HRESULT Ls3ThumbShellExt::SetSite(IUnknown *punkSite) {
  if (_punkSite != nullptr) {
    _punkSite->Release();
    _punkSite = nullptr;
  }
  return punkSite == nullptr ? S_OK : punkSite->QueryInterface(&_punkSite);
}

HRESULT Ls3ThumbShellExt::GetSite(REFIID riid, void **ppv) {
  *ppv = nullptr;
  return _punkSite == nullptr ? E_FAIL : _punkSite->QueryInterface(riid, ppv);
}

// IOleWindow methods
HRESULT Ls3ThumbShellExt::GetWindow(HWND *phwnd) {
  HRESULT hr = E_INVALIDARG;
  if (phwnd) {
    *phwnd = _hwndParent;
    hr = S_OK;
  }
  return hr;
}
