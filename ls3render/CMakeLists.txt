cmake_minimum_required (VERSION 3.3)
project (ls3render LANGUAGES CXX)

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffile-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR")
endif()

add_subdirectory(parser)
generate_zusi_parser(zusi_parser ${CMAKE_CURRENT_BINARY_DIR}/zusi_parser
  WHITELIST
    Zusi::Landschaft

    Landschaft::lsb
    Landschaft::SubSet
    Landschaft::Verknuepfte
    Landschaft::VerknAnimation
    Landschaft::MeshAnimation
    Landschaft::Animation

    Material::C
    Material::CA
    Material::E
    Material::Ca
    Material::Cd
    Material::Ce
    Material::zBias
    Material::Textur
    Material::RenderFlags

    SubSet::NachtEinstellung
    SubSet::MeshI
    SubSet::MeshV
    SubSet::Face
    SubSet::Vertex

    Vertex::p
    Vertex::n

    AnimationsDeklaration::AniID
    AnimationsDeklaration::AniNrs

    AnimationsDefinition::AniNr
    AnimationsDefinition::AniIndex
    AnimationsDefinition::AniPunkt

    AniPunkt::p
    AniPunkt::q

    Textur::Datei

    Verknuepfte::Datei
    Verknuepfte::p
    Verknuepfte::phi
    Verknuepfte::sk

    Dateiverknuepfung::Dateiname

    AniPunkt::AniZeit

    AniNrs::AniNr

    Vertex::U
    Vertex::U2
    Vertex::V
    Vertex::V2

    Face::i

    RenderFlags::TexVoreinstellung

    Vec3::X
    Vec3::Y
    Vec3::Z

    Quaternion::X
    Quaternion::Y
    Quaternion::Z
    Quaternion::W

    Verknuepfte::SichtbarAb
  IGNORE_UNKNOWN)

# TODO: per-target
set (CMAKE_CXX_STANDARD 17)
if (ENABLE_SANITIZERS)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
endif ()

find_package(PkgConfig)
pkg_check_modules(OPENGL REQUIRED gl glew glu)
if (OPENGL_glew_VERSION VERSION_LESS 2.1.0)
  message(FATAL_ERROR "Glew >= 2.1.0 expected, got ${OPENGL_glew_VERSION}")
endif()

find_package(glm QUIET)
if (NOT ${GLM_FOUND})
  pkg_check_modules(glm REQUIRED glm)
endif()

add_library(ls3render STATIC scene.cpp shader.cpp window.cpp)

if (USE_BOOST_FILESYSTEM)
  find_package(Boost COMPONENTS filesystem REQUIRED)
  target_include_directories(ls3render PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(ls3render PRIVATE ${Boost_LIBRARIES})
  target_compile_definitions(ls3render PRIVATE USE_BOOST_FILESYSTEM ZUSI_PARSER_USE_BOOST_FILESYSTEM)
else()
  # TODO: Bei x-compilation wird GCC-Version falsch erkannt
  if (NOT CMAKE_CROSSCOMPILING)
    target_link_libraries(ls3render PRIVATE $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
  endif()
endif()

find_package(Boost 1.73 COMPONENTS nowide REQUIRED)
target_include_directories(ls3render INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ls3render PRIVATE ${OPENGL_INCLUDE_DIRS} ${glm_INCLUDE_DIRS})
target_link_libraries(ls3render PUBLIC ${OPENGL_LIBRARIES} ${glm_LIBRARIES})
target_link_libraries(ls3render PUBLIC zusi_parser)
target_compile_definitions(ls3render PRIVATE -Dls3render_EXPORTS)
target_compile_definitions(ls3render PUBLIC -DGLM_ENABLE_EXPERIMENTAL -DGLEW_STATIC)
target_compile_options(ls3render PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(ls3render PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(ls3render PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
install(TARGETS ls3render DESTINATION bin)
IF (MINGW)
    target_compile_definitions(ls3render PUBLIC -DSTRSAFE_NO_DEPRECATE)
ENDIF (MINGW)
