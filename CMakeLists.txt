project(Ls3Thumb)
cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig)
pkg_check_modules(OPENGL REQUIRED gl glew glu) 
if (OPENGL_glew_VERSION VERSION_LESS 2.1.0)
  message(FATAL_ERROR "Glew >= 2.1.0 expected, got ${OPENGL_glew_VERSION}")
endif()

find_package(glm QUIET)
if (NOT ${GLM_FOUND})
  pkg_check_modules(glm REQUIRED glm)
endif()

add_subdirectory(ls3render)

add_library(ls3thumb SHARED
  class_factory.cpp
  shell_ext.cpp
  dllmain.cpp
  dllmain.def
  )

target_compile_definitions(ls3thumb PRIVATE -DUNICODE)
target_compile_definitions(ls3thumb PRIVATE -D_WIN32_IE=_WIN32_IE_IE70)  # for IPreviewHandlerVisuals to be available
target_compile_definitions(ls3thumb PRIVATE -DGLEW_STATIC)
target_compile_options(ls3thumb PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-error=unused-function)
target_include_directories(ls3thumb PRIVATE ${OPENGL_INCLUDE_DIRS} ${glm_INCLUDE_DIRS})
target_link_libraries(ls3thumb PRIVATE ${OPENGL_LIBRARIES} ${glm_LIBRARIES})
set_target_properties(ls3thumb PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(ls3thumb PROPERTIES CXX_STANDARD 17)
set_target_properties(ls3thumb PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
set_target_properties(ls3thumb PROPERTIES PREFIX "")
target_link_libraries(ls3thumb PRIVATE ls3render)
install(TARGETS ls3thumb DESTINATION bin)

add_executable(ls3thumb_test_thumbnail test_thumbnail.cpp)
set_target_properties(ls3thumb_test_thumbnail PROPERTIES CXX_STANDARD 17)
target_link_libraries(ls3thumb_test_thumbnail PRIVATE ls3thumb gdiplus)
install(TARGETS ls3thumb_test_thumbnail DESTINATION bin)

add_executable(ls3thumb_test_preview test_preview.cpp)
target_compile_definitions(ls3thumb_test_preview PRIVATE -DUNICODE)
set_target_properties(ls3thumb_test_preview PROPERTIES CXX_STANDARD 17)
target_link_libraries(ls3thumb_test_preview PRIVATE ls3thumb -Wl,--subsystem,windows)
install(TARGETS ls3thumb_test_preview DESTINATION bin)
