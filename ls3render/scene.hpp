#pragma once

#include "zusi_parser/utils.hpp"

#include <experimental/propagate_const>
#include <memory>
#include <unordered_map>

#include <glm/mat4x4.hpp>

namespace zusixml {
class ZusiPfad;
}

namespace ls3render {
class ShaderParameters;

class Scene {
public:
  bool LadeLandschaft(const zusixml::ZusiPfad &dateiname,
                      const glm::mat4 &transform,
                      const std::unordered_map<int, float> &ani_positionen,
                      const std::atomic<bool> &abbrechen);
  std::pair<glm::vec3, glm::vec3> GetBoundingBox() const;

  bool LoadIntoGraphicsCardMemory();
  void Render(const ShaderParameters &shader_parameters);
  void FreeGraphicsCardMemory();

  Scene();
  ~Scene();

private:
  struct impl;
  std::experimental::propagate_const<std::unique_ptr<impl>> pImpl;
};
} // namespace ls3render
