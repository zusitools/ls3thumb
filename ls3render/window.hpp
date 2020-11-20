#pragma once

#include <windows.h>

#include <functional>
#include <tuple>

namespace ls3render {

class OpenGlWindow {
public:
  OpenGlWindow(HINSTANCE hInstance, std::pair<size_t, size_t> size);
  ~OpenGlWindow();

  HRESULT SetParent(HWND parent);
  HRESULT SetPositionAndSize(const std::pair<size_t, size_t> &pos,
                             const std::pair<size_t, size_t> &size);
  static HRESULT RegisterWindowClass(HINSTANCE hInstance);

  HRESULT Show();
  HRESULT Hide();
  HRESULT Focus();
  HRESULT Repaint();

  class Context {
  public:
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;

    Context(Context &&) = default;
    Context &operator=(Context &&) = default;

    ~Context();

  private:
    friend class OpenGlWindow;
    Context(const OpenGlWindow &window);

    const OpenGlWindow &_window;
  };
  OpenGlWindow::Context GetContext();

  HRESULT SwapBuffers();

  void SetOnRefresh(std::function<void()> onRefresh) {
    _onRefresh = std::move(onRefresh);
  }
  void SetOnKeyDown(std::function<void(int)> onKeyDown) {
    _onKeyDown = std::move(onKeyDown);
  }
  void SetOnLeftMouseButton(std::function<void(const std::pair<int,int>&, bool)> onLeftMouseButton) {
    _onLeftMouseButton = std::move(onLeftMouseButton);
  }
  void SetOnMouseMove(std::function<void(const std::pair<int,int>&)> onMouseMove) {
    _onMouseMove = std::move(onMouseMove);
  }

private:
  std::function<void()> _onRefresh;
  std::function<void(int)> _onKeyDown;
  std::function<void(const std::pair<int,int>&, bool)> _onLeftMouseButton;
  std::function<void(const std::pair<int,int>&)> _onMouseMove;

  HWND _hwnd{nullptr};
  HGLRC _context{nullptr};
  HDC _dc{nullptr};

  static LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);
};

} // namespace ls3render
