#include "window.hpp"

#include "macros.hpp"

#include <GL/glew.h> // must be included before gl.h
#include <GL/wglew.h>

#include <GL/gl.h>

#include <stdexcept>
#include <string>

#include <windowsx.h>

namespace ls3render {

HRESULT OpenGlWindow::RegisterWindowClass(HINSTANCE hInstance) {
  WNDCLASSEX wcex{};
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = &OpenGlWindow::WinProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.lpszClassName = "Ls3Thumb";

  return RegisterClassEx(&wcex);
}

LRESULT CALLBACK OpenGlWindow::WinProc(HWND hWnd, UINT message, WPARAM wParam,
                                       LPARAM lParam) {
  auto *self =
      reinterpret_cast<OpenGlWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (message) {
  case WM_KEYDOWN:
    if (self->_onKeyDown) {
      self->_onKeyDown(wParam);
    }
    break;
  case WM_LBUTTONDOWN:
    SetCapture(self->_hwnd);
    if (self->_onLeftMouseButton) {
      self->_onLeftMouseButton(
          std::pair{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}, true);
    }
    break;
  case WM_LBUTTONUP:
    ReleaseCapture();
    if (self->_onLeftMouseButton) {
      self->_onLeftMouseButton(
          std::pair{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}, false);
    }
    break;
  case WM_MOUSEMOVE:
    if (self->_onMouseMove) {
      self->_onMouseMove(std::pair{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
    }
    break;
  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  case WM_PAINT:
    if (self->_onRefresh) {
      self->_onRefresh();
    }
    break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

OpenGlWindow::OpenGlWindow(HINSTANCE hInstance,
                           std::pair<size_t, size_t> size) {
  HWND fakeWND = CreateWindow("Ls3Thumb", "Fake Window", // window class, title
                              WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // style
                              0, 0,                // position x, y
                              1, 1,                // width, height
                              nullptr, nullptr,    // parent window, menu
                              hInstance, nullptr); // instance, param

  HDC fakeDC = GetDC(fakeWND); // Device Context

  PIXELFORMATDESCRIPTOR fakePFD{};
  fakePFD.nSize = sizeof(fakePFD);
  fakePFD.nVersion = 1;
  fakePFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  fakePFD.iPixelType = PFD_TYPE_RGBA;
  fakePFD.cColorBits = 32;
  fakePFD.cAlphaBits = 8;
  fakePFD.cDepthBits = 24;

  const int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);
  if (fakePFDID == 0) {
    throw std::runtime_error("ChoosePixelFormat() failed.");
  }

  if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == false) {
    throw std::runtime_error("SetPixelFormat() failed.");
  }

  HGLRC fakeRC = wglCreateContext(fakeDC); // Rendering Contex

  if (fakeRC == 0) {
    throw std::runtime_error("wglCreateContext() failed.");
  }

  TRY(wglMakeCurrent(fakeDC, fakeRC));

  // get pointers to functions (or init opengl loader here)
  TRY_GLEW(glewInit());

  // create a new window and context
  _hwnd =
      CreateWindow("Ls3Thumb", "OpenGL Window", // class name, window name
                   WS_POPUP,                    // styles
                   0, 0, size.first, size.second, nullptr,
                   nullptr,             // parent window, menu
                   hInstance, nullptr); // instance, param
  SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  _dc = GetDC(_hwnd);

  const int pixelAttribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB, GL_TRUE, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, WGL_COLOR_BITS_ARB, 32,
      WGL_ALPHA_BITS_ARB, 8, WGL_DEPTH_BITS_ARB, 16,
      // WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
      // WGL_SAMPLES_ARB, 4,
      0};

  int pixelFormatID;
  UINT numFormats;
  TRY(wglChoosePixelFormatARB(_dc, pixelAttribs, nullptr, 1, &pixelFormatID,
                              &numFormats));

  if (numFormats == 0) {
    throw std::runtime_error("wglChoosePixelFormatARB() failed.");
  }

  PIXELFORMATDESCRIPTOR PFD;
  DescribePixelFormat(_dc, pixelFormatID, sizeof(PFD), &PFD);
  SetPixelFormat(_dc, pixelFormatID, &PFD);

  const int contextAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                                3,
                                WGL_CONTEXT_MINOR_VERSION_ARB,
                                2,
                                WGL_CONTEXT_PROFILE_MASK_ARB,
                                WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifndef NDEBUG
                                WGL_CONTEXT_FLAGS_ARB,
                                WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                                0};

  _context = wglCreateContextAttribsARB(_dc, 0, contextAttribs);
  if (_context == nullptr) {
    throw std::runtime_error("wglCreateContextAttribsARB() failed.");
  }

  // delete temporary context and window

  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(fakeRC);
  ReleaseDC(fakeWND, fakeDC);
  DestroyWindow(fakeWND);
}

OpenGlWindow::~OpenGlWindow() {
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(_context);
  ReleaseDC(_hwnd, _dc);
  DestroyWindow(_hwnd);
}

HRESULT OpenGlWindow::SetParent(HWND parent) {
  long style = GetWindowLong(_hwnd, GWL_STYLE);
  if (parent == nullptr) {
    style &= ~WS_CHILDWINDOW;
    style |= WS_POPUP;
    ::SetParent(_hwnd, parent);
    SetWindowLong(_hwnd, GWL_STYLE, style);
  } else {
    style &= ~WS_POPUP;
    style |= WS_CHILDWINDOW;
    SetWindowLong(_hwnd, GWL_STYLE, style);
    ::SetParent(_hwnd, parent);
  }
  return S_OK;
}
HRESULT
OpenGlWindow::SetPositionAndSize(const std::pair<size_t, size_t> &pos,
                                 const std::pair<size_t, size_t> &size) {
  return SetWindowPos(_hwnd, nullptr, pos.first, pos.second, size.first,
                      size.second, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}
HRESULT OpenGlWindow::Show() { return ShowWindow(_hwnd, SW_SHOWNA); }
HRESULT OpenGlWindow::Hide() { return ShowWindow(_hwnd, SW_HIDE); }
HRESULT OpenGlWindow::Focus() {
  return SetFocus(_hwnd) != nullptr ? S_OK : E_FAIL;
}
HRESULT OpenGlWindow::Repaint() {
  return InvalidateRect(_hwnd, nullptr, true);
}

OpenGlWindow::Context::Context(const OpenGlWindow &window) : _window(window) {
  if (!wglMakeCurrent(window._dc, window._context)) {
    throw std::runtime_error("wglMakeCurrent() failed.");
  }
}
OpenGlWindow::Context::~Context() { wglMakeCurrent(nullptr, nullptr); }

OpenGlWindow::Context OpenGlWindow::GetContext() {
  return OpenGlWindow::Context(*this);
}

HRESULT OpenGlWindow::SwapBuffers() { return ::SwapBuffers(_dc); }
} // namespace ls3render
