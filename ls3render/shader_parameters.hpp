#pragma once

#include <GL/glew.h> // needs to be included before gl.h

#include <GL/gl.h>

#include <iostream>
#include <vector>

#include <windows.h>

namespace ls3render {

struct ShaderParameters {
  GLint attrib_pos;
  GLint attrib_nor;
  GLint attrib_uv1;
  GLint attrib_uv2;
  GLint uni_nor;
  GLint uni_model;
  GLint uni_view;
  GLint uni_proj;
  std::vector<GLint> uni_tex;
  GLint uni_numTextures;
  GLint uni_color;
  GLint uni_alphaCutoff;
  GLint uni_texVoreinstellung;

  ShaderParameters() : uni_tex() {}

  void validate() const {
#ifdef UNICODE
#define CHECK_MINUS_ONE(a)                                                     \
  do {                                                                         \
    if ((a) == -1) {                                                           \
      OutputDebugString(L"Warning: Shader parameter " #a L" is -1\r\n");       \
    }                                                                          \
  } while (0);
#else
#define CHECK_MINUS_ONE(a)                                                     \
  do {                                                                         \
    if ((a) == -1) {                                                           \
      OutputDebugString("Warning: Shader parameter " #a " is -1\r\n");         \
    }                                                                          \
  } while (0);
#endif

    CHECK_MINUS_ONE(attrib_pos);
    CHECK_MINUS_ONE(attrib_nor);
    CHECK_MINUS_ONE(attrib_uv1);
    CHECK_MINUS_ONE(attrib_uv2);
    CHECK_MINUS_ONE(uni_nor);
    CHECK_MINUS_ONE(uni_model);
    CHECK_MINUS_ONE(uni_view);
    CHECK_MINUS_ONE(uni_proj);
    for (size_t i = 0; i < uni_tex.size(); i++) {
      CHECK_MINUS_ONE(uni_tex[i]);
    }
    CHECK_MINUS_ONE(uni_numTextures);
    CHECK_MINUS_ONE(uni_color);
    CHECK_MINUS_ONE(uni_alphaCutoff);
    CHECK_MINUS_ONE(uni_texVoreinstellung);
  }

#undef CHECK_MINUS_ONE
};

} // namespace ls3render
