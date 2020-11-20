#define INITGUID
#include <initguid.h>

#include "thumbcache.h"
#include <iostream>
#include <propsys.h>
#include <windows.h>

#include "debug.hpp"
#include "guid.hpp"

#include <boost/nowide/convert.hpp>

IPreviewHandler *pIPH = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
    case WM_CREATE:
      break;
    case WM_DESTROY:
      if (pIPH != nullptr) {
        pIPH->Unload();
      }
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine,
                   int nCmdShow) {
  int argc;
  auto **argv = CommandLineToArgvW(GetCommandLine(), &argc);

  // Register the window class.
  const wchar_t CLASS_NAME[] = L"Sample Window Class";

  WNDCLASS wc = {};

  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClass(&wc);

  // Create the window.

  HWND hwnd =
      CreateWindowEx(0,                            // Optional window styles.
                     CLASS_NAME,                   // Window class
                     L"Learn to Program Windows",  // Window text
                     WS_OVERLAPPEDWINDOW,          // Window style

                     // Size and position
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                     NULL,       // Parent window
                     NULL,       // Menu
                     hInstance,  // Instance handle
                     NULL        // Additional application data
      );

  if (hwnd == NULL) {
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);

#define TRY(doSomething)                                                   \
  {                                                                        \
    do {                                                                   \
      HRESULT hr = doSomething;                                            \
      if (SUCCEEDED(hr)) {                                                 \
        std::cerr << "SUCCESS: " #doSomething "\n";                        \
      } else {                                                             \
        std::cerr << "FAIL: " #doSomething ": " << std::hex << hr << "\n"; \
        return 1;                                                          \
      }                                                                    \
    } while (0);                                                           \
  }

  TRY(CoInitialize(nullptr));

  IUnknown *pIU;
  TRY(CoCreateInstance(CLSID_Ls3ThumbShlExt, nullptr, CLSCTX_INPROC_SERVER,
                       IID_IUnknown, reinterpret_cast<void **>(&pIU)));

  IInitializeWithFile *pIWF;
  TRY(pIU->QueryInterface(IID_IInitializeWithFile,
                          reinterpret_cast<void **>(&pIWF)));
  TRY(pIWF->Initialize(argv[1], STGM_READ));

  TRY(pIU->QueryInterface(IID_IPreviewHandler,
                          reinterpret_cast<void **>(&pIPH)));

  RECT r;
  TRY(GetWindowRect(hwnd, &r));
  TRY(pIPH->SetWindow(hwnd, &r));
  TRY(pIPH->DoPreview());

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  TRY(pIPH->Release());
  TRY(pIWF->Release());
  TRY(pIU->Release());
  CoUninitialize();

  LocalFree(argv);
  return 0;
}
