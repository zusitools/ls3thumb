#pragma once

#include <sstream>

#include <windows.h>

#ifdef UNICODE
#define WIDEN(x) L##x L"\r\n"
#else
#define WIDEN(x) x "\r\n"
#endif

#define TRY(glDoSomething)                                                     \
  {                                                                            \
    do {                                                                       \
      /*OutputDebugString(WIDEN(#glDoSomething));*/                            \
      glDoSomething;                                                           \
      GLenum error = glGetError();                                             \
      if (error == GL_NO_ERROR) {                                              \
        break;                                                                 \
      }                                                                        \
      std::stringstream msg;                                                   \
      do {                                                                     \
        msg << __FILE__ ":" << std::dec << __LINE__                            \
            << ": Error executing " #glDoSomething ": 0x" << std::hex << error \
            << '\n';                                                           \
        error = glGetError();                                                  \
      } while (error != GL_NO_ERROR);                                          \
      throw std::runtime_error(msg.str());                                     \
    } while (0);                                                               \
  }

#define TRY_GLEW(glewDoSomething)                                              \
  {                                                                            \
    do {                                                                       \
      /*OutputDebugString(WIDEN(#glewDoSomething));*/                          \
      GLenum error = glewDoSomething;                                          \
      if (error != GLEW_OK) {                                                  \
        std::stringstream msg;                                                 \
        msg << __FILE__ ":" << std::dec << __LINE__                            \
            << ": Error executing " #glewDoSomething ": "                      \
            << glewGetErrorString(error) << '\n';                              \
        throw std::runtime_error(msg.str());                                   \
      }                                                                        \
    } while (0);                                                               \
  }
