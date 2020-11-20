#pragma once

#include <boost/nowide/convert.hpp>
#include <boost/nowide/stackstring.hpp>

#include <windows.h>

#include <sstream>

// https://stackoverflow.com/a/14422777
#define STRINGIFY2(m) #m
#define STRINGIFY(m) STRINGIFY2(m)

#define DEBUG(args)                                                       \
  {                                                                       \
    do {                                                                  \
      std::stringstream msg;                                              \
      msg << "[T " << GetCurrentThreadId() << "] " << __PRETTY_FUNCTION__ \
          << ":" STRINGIFY(__LINE__) ": " << args << "\r\n";              \
      OutputDebugString(boost::nowide::widen(msg.str()).c_str());         \
    } while (0);                                                          \
  }

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits> &operator<<(
    std::basic_ostream<CharT, Traits> &out, const GUID &guid) {
  OLECHAR *guidString;
  StringFromCLSID(guid, &guidString);
  boost::nowide::basic_stackstring<char, OLECHAR, 38> temp;
  temp.convert(guidString);
  ::CoTaskMemFree(guidString);
  out << temp.get();
  return out;
}
