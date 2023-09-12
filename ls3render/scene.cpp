#include "scene.hpp"

#include <atomic>
#include <fstream>
#include <memory>
#include <optional>

#include "./render_object.hpp"
#include "./utils.hpp"
#include "../debug.hpp"

#include "zusi_parser/utils.hpp"
#include "zusi_parser/zusi_parser.hpp"
#include "zusi_parser/zusi_types.hpp"

#if USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#endif

namespace ls3render {

class Scene::impl {
 public:
  bool LadeLandschaft(const zusixml::ZusiPfad &dateiname,
                      const glm::mat4 &transform,
                      const std::unordered_map<int, float> &ani_positionen,
                      const std::atomic<bool> &abbrechen) {
    if (abbrechen.load()) {
      return false;
    }

    const auto &dateinameOsPfad = dateiname.alsOsPfad();
    std::unique_ptr<Zusi> zusi_datei = zusixml::tryParseFile(dateinameOsPfad);
    if (!zusi_datei) {
      DEBUG("Error parsing file");
      DEBUG(dateinameOsPfad);
      return false;
    }
    auto *landschaft = zusi_datei->Landschaft.get();
    if (landschaft) {
      if (!landschaft->lsb.Dateiname.empty()) {
        const std::string lsb_pfad =
            zusixml::ZusiPfad::vonZusiPfad(landschaft->lsb.Dateiname, dateiname)
                .alsOsPfad();

        std::ifstream lsb_stream;
        lsb_stream.exceptions(std::ifstream::failbit | std::ifstream::eofbit |
                              std::ifstream::badbit);
        try {
          lsb_stream.open(lsb_pfad, std::ios::in | std::ios::binary);
        } catch (const std::ifstream::failure &e) {
          std::cerr << lsb_pfad << ": open() failed: " << e.what();
          return false;
        }

        try {
          for (const auto &mesh_subset : landschaft->children_SubSet) {
            static_assert(sizeof(Vertex) == 40, "Wrong size of Vertex");
            static_assert(offsetof(Vertex, p) == 0,
                          "Wrong offset of Vertex::p");
            static_assert(offsetof(Vertex, n) == 12,
                          "Wrong offset of Vertex::n");
            static_assert(offsetof(Vertex, U) == 24,
                          "Wrong offset of Vertex::U");
            static_assert(offsetof(Vertex, V) == 28,
                          "Wrong offset of Vertex::V");
            static_assert(offsetof(Vertex, U2) == 32,
                          "Wrong offset of Vertex::U2");
            static_assert(offsetof(Vertex, V2) == 36,
                          "Wrong offset of Vertex::V2");
            mesh_subset->children_Vertex.resize(mesh_subset->MeshV);
            lsb_stream.read(
                reinterpret_cast<char *>(mesh_subset->children_Vertex.data()),
                mesh_subset->children_Vertex.size() * sizeof(Vertex));

            static_assert(sizeof(Face) == 6, "Wrong size of Face");
            assert(mesh_subset->MeshI % 3 == 0);
            mesh_subset->children_Face.resize(mesh_subset->MeshI / 3);
            lsb_stream.read(
                reinterpret_cast<char *>(mesh_subset->children_Face.data()),
                mesh_subset->children_Face.size() * sizeof(Face));
          }
        } catch (const std::ifstream::failure &e) {
          DEBUG("Error reading LSB file");
          DEBUG(e.what());
          return false;
        }

        lsb_stream.exceptions(std::ios_base::iostate());
        lsb_stream.peek();
        assert(lsb_stream.eof());
      }
    } else if (auto *formkurve = zusi_datei->Formkurve.get()) {
      zusi_datei->Landschaft = std::make_unique<Landschaft>();
      landschaft = zusi_datei->Landschaft.get();

      auto subset =
          std::make_unique<SubSet>(SubSet{std::move(*formkurve->Material)});
      subset->children_Vertex.reserve(2 * formkurve->children_Punkt.size());
      subset->children_Face.reserve(2 * formkurve->children_Punkt.size());

      const auto len = std::max(subset->MeterProTex, subset->MeterProTex2);

      Vertex musterVertex{};
      for (const auto &punkt : formkurve->children_Punkt) {
        musterVertex.p.X = punkt->X;
        musterVertex.p.Y = -len / 2.0;
        musterVertex.p.Z = punkt->Y;
        musterVertex.U = formkurve->TexIstU ? punkt->tex : 0;
        musterVertex.U2 = formkurve->TexIstU ? punkt->tex : 0;
        musterVertex.V = formkurve->TexIstU ? 0 : punkt->tex;
        musterVertex.V2 = formkurve->TexIstU ? 0 : punkt->tex;
        musterVertex.n.X = punkt->nX;
        musterVertex.n.Y = 1;
        musterVertex.n.Z = punkt->nY;
        subset->children_Vertex.emplace_back(musterVertex);

        musterVertex.p.Y = len / 2.0;
        (formkurve->TexIstU ? musterVertex.V : musterVertex.U) =
            (subset->MeterProTex == 0 ? 0 : len / subset->MeterProTex);
        (formkurve->TexIstU ? musterVertex.V2 : musterVertex.U2) =
            (subset->MeterProTex2 == 0 ? 0 : len / subset->MeterProTex2);
        subset->children_Vertex.emplace_back(musterVertex);
      }
      for (size_t i = 0; i + 1 < formkurve->children_Punkt.size(); ++i) {
        subset->children_Face.push_back(
            Face{{2 * i + 0, 2 * i + 1, 2 * i + 2}});
        subset->children_Face.push_back(
            Face{{2 * i + 3, 2 * i + 2, 2 * i + 1}});
      }
      landschaft->children_SubSet.push_back(std::move(subset));
    } else {
      DEBUG("Keine Zusi-Datei");
      return false;
    }

    if (abbrechen.load()) {
      return false;
    }

    for (auto &mesh_subset : landschaft->children_SubSet) {
      for (auto &textur : mesh_subset->children_Textur) {
        if (!textur->Datei.Dateiname.empty()) {
          textur->Datei.Dateiname =
              zusixml::ZusiPfad::vonZusiPfad(textur->Datei.Dateiname, dateiname)
                  .alsOsPfad();
        }
      }
    }

    m_Ls3Dateien.push_back(std::move(zusi_datei));  // keep for later

    auto render_object =
        std::make_unique<Ls3RenderObject>(*landschaft, ani_positionen);
    render_object->setTransform(transform);
    m_RenderObjects.push_back(std::move(render_object));

    for (size_t counter = 0, len = landschaft->children_Verknuepfte.size();
         counter < len; counter++) {
      if (abbrechen.load()) {
        return false;
      }

      // Zusi zeichnet verknuepfte Dateien mit dem gleichen Abstand zur Kamera
      // in der _umgekehrten_ Reihenfolge, wie sie in der LS3-Datei stehen.
      // Jede Datei bekommt einen Abstandsbonus in Hoehe der doppelten Summe der
      // Subset-Z-Offsets. Wir nehmen vereinfachend an, dass alle Objekte
      // denselben Abstand zur Kamera haben. Der Z-Offset wird spaeter vor dem
      // Rendern beruecksichtigt.
      size_t i = len - 1 - counter;
      const auto &verkn = landschaft->children_Verknuepfte[i];

      if (verkn->SichtbarAb > 0) {
        continue;
      }

      const auto verkn_dateiname =
          zusixml::ZusiPfad::vonZusiPfad(verkn->Datei.Dateiname, dateiname);
      if (verkn->Datei.Dateiname.empty()) {
        continue;
      }
      std::experimental::optional<AniPunkt> verkn_animation;

      for (const auto &v : landschaft->children_VerknAnimation) {
        std::experimental::optional<float> t;
        if (v->AniIndex >= 0 && static_cast<decltype(i)>(v->AniIndex) == i) {
          for (const auto &def : landschaft->children_Animation) {
            if (std::find_if(std::begin(def->children_AniNrs),
                             std::end(def->children_AniNrs),
                             [&v](const auto &aniNrs) {
                               return aniNrs->AniNr == v->AniNr;
                             }) != std::end(def->children_AniNrs)) {
              auto it = ani_positionen.find(def->AniID);
              if (it != std::end(ani_positionen)) {
                t = it->second;
              } else {
              }
              verkn_animation =
                  interpoliere(v->children_AniPunkt, t.value_or(0.0f));
              break;
            }
          }
          if (verkn_animation) {
            break;
          }
        }
      }

      glm::mat4 rot_verkn =
          glm::eulerAngleXYZ(verkn->phi.X, verkn->phi.Y, verkn->phi.Z);
      glm::mat4 transform_verkn = rot_verkn;

      if (verkn_animation) {
        transform_verkn =
            glm::toMat4(glm::quat(verkn_animation->q.W, verkn_animation->q.X,
                                  verkn_animation->q.Y, verkn_animation->q.Z)) *
            transform_verkn;
      }

      auto zero_to_one = [](float f) { return f == 0.0 ? 1.0 : f; };
      transform_verkn =
          glm::scale(glm::mat4{1}, glm::vec3(zero_to_one(verkn->sk.X),
                                             zero_to_one(verkn->sk.Y),
                                             zero_to_one(verkn->sk.Z))) *
          transform_verkn;
      transform_verkn =
          glm::translate(glm::mat4{1},
                         glm::vec3(verkn->p.X, verkn->p.Y, verkn->p.Z)) *
          transform_verkn;

      if (verkn_animation) {
        // Translation der Verknuepfungsanimation bezieht sich auf das durch die
        // Verknuepfung rotierte Koordinatensystem.
        glm::mat4 translate_verkn_animation =
            rot_verkn *
            glm::translate(glm::mat4{1},
                           glm::vec3(verkn_animation->p.X, verkn_animation->p.Y,
                                     verkn_animation->p.Z)) *
            glm::inverse(rot_verkn);
        transform_verkn = translate_verkn_animation * transform_verkn;
      }

      this->LadeLandschaft(verkn_dateiname, transform * transform_verkn,
                           ani_positionen, abbrechen);
    }

    return true;
  }

  std::pair<glm::vec3, glm::vec3> GetBoundingBox() const {
    std::pair<glm::vec3, glm::vec3> result{
        glm::vec3{std::numeric_limits<glm::vec3::value_type>::max(),
                  std::numeric_limits<glm::vec3::value_type>::max(),
                  std::numeric_limits<glm::vec3::value_type>::max()},
        glm::vec3{std::numeric_limits<glm::vec3::value_type>::min(),
                  std::numeric_limits<glm::vec3::value_type>::min(),
                  std::numeric_limits<glm::vec3::value_type>::min()}};
    for (const auto &ro : m_RenderObjects) {
      ro->updateBoundingBox(&result);
    }
    return result;
  }

  bool LoadIntoGraphicsCardMemory() {
    for (const auto &ro : m_RenderObjects) {
      if (!ro->init()) {
        std::cerr << "Error initializing render object\n";
        return false;
      }
    }
    return true;
  }

  void Render(const ShaderParameters &shader_parameters) {
    // Sortiere nach -zOffsetSumme, damit negativer Z-Offset => spaeter zeichnen
    std::stable_sort(std::begin(m_RenderObjects), std::end(m_RenderObjects),
                     [](const auto &lhs, const auto &rhs) {
                       // lhs < rhs
                       return -lhs->getSubsetZOffsetSumme() <
                              -rhs->getSubsetZOffsetSumme();
                     });
    for (const auto &render_object : m_RenderObjects) {
      assert(render_object != nullptr);
      render_object->render(shader_parameters);
    }
  }

  void FreeGraphicsCardMemory() {
    for (const auto &ro : m_RenderObjects) {
      ro->cleanup();
    }
  }

  impl() : m_Ls3Dateien(), m_RenderObjects() {
  }

 private:
  std::vector<std::unique_ptr<Zusi>> m_Ls3Dateien;
  std::vector<std::unique_ptr<RenderObject>> m_RenderObjects;
};

Scene::Scene() : pImpl{std::make_unique<impl>()} {
}
Scene::~Scene() = default;

bool Scene::LadeLandschaft(const zusixml::ZusiPfad &dateiname,
                           const glm::mat4 &transform,
                           const std::unordered_map<int, float> &ani_positionen,
                           const std::atomic<bool> &abbrechen) {
  return pImpl->LadeLandschaft(dateiname, transform, ani_positionen, abbrechen);
}
std::pair<glm::vec3, glm::vec3> Scene::GetBoundingBox() const {
  return pImpl->GetBoundingBox();
}
bool Scene::LoadIntoGraphicsCardMemory() {
  return pImpl->LoadIntoGraphicsCardMemory();
}
void Scene::Render(const ShaderParameters &shader_parameters) {
  pImpl->Render(shader_parameters);
}
void Scene::FreeGraphicsCardMemory() {
  pImpl->FreeGraphicsCardMemory();
}

}  // namespace ls3render
