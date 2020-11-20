#pragma once

#include "zusi_parser/zusi_types.hpp"

#include <algorithm>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

std::experimental::optional<AniPunkt>
interpoliere(const std::vector<std::unique_ptr<AniPunkt>> &ani_punkte,
             float t) {
  if (ani_punkte.size() == 0) {
    return std::experimental::nullopt;
  }
  auto rechts =
      std::upper_bound(std::begin(ani_punkte), std::end(ani_punkte), t,
                       [](float t, const auto &a) { return t < a->AniZeit; });
  if (rechts == std::begin(ani_punkte)) {
    return AniPunkt(**std::begin(ani_punkte));
  } else {
    auto links = std::prev(rechts);
    if (rechts == std::end(ani_punkte) ||
        (*rechts)->AniZeit == (*links)->AniZeit) {
      return AniPunkt(**links);
    } else {
      assert((*rechts)->AniZeit > (*links)->AniZeit);
      float factor =
          (t - (*links)->AniZeit) / ((*rechts)->AniZeit - (*links)->AniZeit);
      glm::quat rot = glm::slerp(
          glm::quat((*links)->q.W, (*links)->q.X, (*links)->q.Y, (*links)->q.Z),
          glm::quat((*rechts)->q.W, (*rechts)->q.X, (*rechts)->q.Y,
                    (*rechts)->q.Z),
          factor);

      AniPunkt result;
      result.AniZeit = t;
      result.p = Vec3{(1 - factor) * (*links)->p.X + factor * (*rechts)->p.X,
                      (1 - factor) * (*links)->p.Y + factor * (*rechts)->p.Y,
                      (1 - factor) * (*links)->p.Z + factor * (*rechts)->p.Z};
      result.q = Quaternion{rot.w, rot.x, rot.y, rot.z};
      return result;
    }
  }
}
