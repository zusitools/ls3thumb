#include "shader.hpp"

#include <GL/glew.h> // needs to be included before gl.h

#include "macros.hpp"
#include "shader_parameters.hpp"

namespace {
static const std::string vs_source =
#include "./assets/vertex_shader.glsl"
    ;

static const std::string fs_source =
#include "./assets/fragment_shader.glsl"
    ;
} // namespace

namespace ls3render {

struct ShaderManager::impl {
  ShaderParameters m_ShaderParameters;

  impl() {
    // Load shaders
    GLuint vertex_shader;
    TRY(vertex_shader = glCreateShader(GL_VERTEX_SHADER));
    const char *c_str = vs_source.c_str();
    TRY(glShaderSource(vertex_shader, 1, &c_str, nullptr));

    GLuint fragment_shader;
    TRY(fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));
    c_str = fs_source.c_str();
    TRY(glShaderSource(fragment_shader, 1, &c_str, nullptr));

    // Compile shaders
    TRY(glCompileShader(vertex_shader));

    GLint buflen;
    TRY(glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &buflen));
    if (buflen > 1) {
      std::string buf;
      buf.resize(buflen);
      glGetShaderInfoLog(vertex_shader, buflen, nullptr, buf.data());
      std::cerr << "Compiling vertex shader produced warnings or errors: "
                << buf << "\n";
    }

    GLint status;
    TRY(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status));
    if (status != GL_TRUE) {
      throw std::runtime_error("Compiling vertex shader failed.");
    }

    TRY(glCompileShader(fragment_shader));

    TRY(glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &buflen));
    if (buflen > 1) {
      std::string buf;
      buf.resize(buflen);
      glGetShaderInfoLog(fragment_shader, buflen, nullptr, buf.data());
      std::cerr << "Compiling fragment shader produced warnings or errors: "
                << buf << "\n";
    }

    TRY(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status));
    if (status != GL_TRUE) {
      throw std::runtime_error("Compiling fragment shader failed.");
    }

    // Combine vertex and fragment shaders into a program
    GLuint shader_program = glCreateProgram();
    TRY(glAttachShader(shader_program, vertex_shader));
    TRY(glAttachShader(shader_program, fragment_shader));

    // Bind output of fragment shader to buffer
    // TODO: only after creating frame-/renderbuffers?
    TRY(glBindFragDataLocation(
        shader_program, 0,
        "outColor")); // not necessary because there is only one output

    // Link and use program
    TRY(glLinkProgram(shader_program));
    TRY(glUseProgram(shader_program));

    m_ShaderParameters.attrib_pos =
        glGetAttribLocation(shader_program, "position");
    m_ShaderParameters.attrib_nor =
        glGetAttribLocation(shader_program, "normal");
    m_ShaderParameters.attrib_uv1 = glGetAttribLocation(shader_program, "uv1");
    m_ShaderParameters.attrib_uv2 = glGetAttribLocation(shader_program, "uv2");

    // Bind uniform variables
    m_ShaderParameters.uni_nor = glGetUniformLocation(shader_program, "nor");
    m_ShaderParameters.uni_model =
        glGetUniformLocation(shader_program, "model");
    m_ShaderParameters.uni_view = glGetUniformLocation(shader_program, "view");
    m_ShaderParameters.uni_proj = glGetUniformLocation(shader_program, "proj");
    m_ShaderParameters.uni_tex.push_back(
        glGetUniformLocation(shader_program, "tex1"));
    m_ShaderParameters.uni_tex.push_back(
        glGetUniformLocation(shader_program, "tex2"));
    m_ShaderParameters.uni_numTextures =
        glGetUniformLocation(shader_program, "numTextures");
    m_ShaderParameters.uni_color =
        glGetUniformLocation(shader_program, "color");
    m_ShaderParameters.uni_alphaCutoff =
        glGetUniformLocation(shader_program, "alphaCutoff");
    m_ShaderParameters.uni_texVoreinstellung =
        glGetUniformLocation(shader_program, "texVoreinstellung");

    m_ShaderParameters.validate();
  }
};

ShaderManager::ShaderManager() : pImpl{std::make_unique<impl>()} {}
ShaderManager::~ShaderManager() = default;
const ShaderParameters &ShaderManager::getShaderParameters() const {
  return pImpl->m_ShaderParameters;
}

} // namespace ls3render
