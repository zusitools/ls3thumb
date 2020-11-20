#define INITGUID
#include <initguid.h>

#include "thumbcache.h"
#include <iostream>
#include <propsys.h>
#include <windows.h>

#include "guid.hpp"

#include <gdiplus.h>
using namespace Gdiplus;

#include <boost/nowide/args.hpp>
#include <boost/nowide/convert.hpp>

#include <cassert>
#include <optional>
#include <string_view>
#include <vector>

void errhandler(LPCSTR s, HANDLE) {
  std::wcerr << s << '\n';
}

std::optional<CLSID> GetEncoderClsid(std::string_view format) {
  UINT num = 0;   // number of image encoders
  UINT size = 0;  // size of the image encoder array in bytes

  GetImageEncodersSize(&num, &size);
  if (size == 0) {
    return std::nullopt;
  }

  std::vector<byte> imageCodecInfos(size);
  Gdiplus::ImageCodecInfo *pImageCodecInfos =
      reinterpret_cast<ImageCodecInfo *>(imageCodecInfos.data());
  GetImageEncoders(num, imageCodecInfos.size(), pImageCodecInfos);

  const auto formatWide = boost::nowide::widen(format.data(), format.size());
  for (size_t i = 0; i < num; ++i) {
    const auto &encoder = pImageCodecInfos[i];
    if (formatWide == encoder.MimeType) {
      return encoder.Clsid;
    }
  }

  return std::nullopt;
}

int main(int argc, char **argv) {
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

  boost::nowide::args a(argc, argv);
  TRY(CoInitialize(nullptr));

  IUnknown *pIU;
  TRY(CoCreateInstance(CLSID_Ls3ThumbShlExt, nullptr, CLSCTX_INPROC_SERVER,
                       IID_IUnknown, reinterpret_cast<void **>(&pIU)));

  IInitializeWithFile *pIWF;
  TRY(pIU->QueryInterface(IID_IInitializeWithFile,
                          reinterpret_cast<void **>(&pIWF)));
  TRY(pIWF->Initialize(boost::nowide::widen(argv[1]).c_str(), STGM_READ));

  IThumbnailProvider *pITP;
  TRY(pIU->QueryInterface(IID_IThumbnailProvider,
                          reinterpret_cast<void **>(&pITP)));
  HBITMAP hb;
  WTS_ALPHATYPE at;
  TRY(pITP->GetThumbnail(253, &hb, &at));

  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

  {
    Bitmap image(hb, nullptr);

    const auto clsid = GetEncoderClsid("image/bmp");
    if (!clsid.has_value()) {
      return 1;
    }

    image.Save(L"output.bmp", &*clsid, nullptr);
  }

  GdiplusShutdown(gdiplusToken);

  TRY(pIWF->Release());
  TRY(pITP->Release());
  TRY(pIU->Release());
  CoUninitialize();
}
