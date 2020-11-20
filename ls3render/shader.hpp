#pragma once

#include <experimental/propagate_const>
#include <memory>

namespace ls3render {

struct ShaderParameters;

class ShaderManager {
public:
  ShaderManager();
  ~ShaderManager();
  const ShaderParameters &getShaderParameters() const;

private:
  struct impl;
  std::experimental::propagate_const<std::unique_ptr<impl>> pImpl;
};

} // namespace ls3render
