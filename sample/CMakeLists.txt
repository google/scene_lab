# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 2.8.12)

project(scene_lab_sample)

if("${CMAKE_CURRENT_LIST_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # We are building from the sample directory, so we need to set up some
  # dependencies first.

  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  # Temporary files (like object files) created while compiling projects.
  set(tmp_dir ${CMAKE_BINARY_DIR}/obj)

  # Directory which contains the source for 3rd party libraries.
  if(NOT DEFINED third_party_root)
    get_filename_component(
      third_party_root "${CMAKE_CURRENT_LIST_DIR}/../../../../../external"
      REALPATH)
  endif()

  # Directory which contains source for FPL libraries.
  if(NOT DEFINED fpl_root)
    get_filename_component(
      fpl_root "${CMAKE_CURRENT_LIST_DIR}/../../../libs" REALPATH)
  endif()

  # If the dependencies directory exists, assume this is the root directory for
  # all libraries required by this project.
  if(NOT DEFINED dependencies_root)
    set(dependencies_root "${CMAKE_CURRENT_LIST_DIR}/../dependencies")
    if(EXISTS "${dependencies_root}")
      set(third_party_root "${dependencies_root}")
      set(fpl_root "${dependencies_root}")
    else()
      set(dependencies_root "${CMAKE_CURRENT_LIST_DIR}/dependencies")
      if(EXISTS "${dependencies_root}")
        set(third_party_root "${dependencies_root}")
        set(fpl_root "${dependencies_root}")
      endif()
    endif()
  endif()

  # Configurable locations of dependencies of this project.
  set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
  set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
  set(dependencies_pindrop_dir "${fpl_root}/pindrop"
    CACHE PATH "Directory containing the pindrop library.")
  set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
  set(dependencies_corgi_dir "${fpl_root}/corgi"
    CACHE PATH "Directory containing the CORGI library.")
  set(dependencies_flatui_dir "${fpl_root}/flatui"
    CACHE PATH "Directory containing the FlatUI library.")
  set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
  set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")
  if (EXISTS ${fpl_root}/scene_lab)
    set(dependencies_scene_lab_dir "${fpl_root}/scene_lab"
      CACHE PATH "Directory containing the Scene Lab library.")
  else()
    set(dependencies_scene_lab_dir "${CMAKE_CURRENT_LIST_DIR}/.."
      CACHE PATH "Directory containing the Scene Lab library.")
  endif()
  set(dependencies_corgi_component_library_dir
    "${dependencies_corgi_dir}/component_library"
    CACHE PATH "Directory containing the CORGI component library")

  if(NOT TARGET fplbase)
    # Include FPLBase in this project.
    set(fplbase_build_mesh_pipeline OFF CACHE BOOL "")
    add_subdirectory("${dependencies_fplbase_dir}" ${tmp_dir}/fplbase)
  endif()

  # Include FlatBuffers in this project.
  if(NOT TARGET flatc)
    set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
    add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)
  endif()

  if (NOT TARGET mathfu)
    # Include MathFu without tests or benchmarks.
    set(mathfu_build_benchmarks OFF CACHE BOOL "")
    set(mathfu_build_tests OFF CACHE BOOL "")
    add_subdirectory(${dependencies_mathfu_dir} ${tmp_dir}/mathfu)
  endif()

  if(NOT TARGET corgi)
    # Include CORGI.
    add_subdirectory("${dependencies_corgi_dir}" ${tmp_dir}/corgi)
  endif()

  if(NOT TARGET flatui)
    # Include FlatUI UI system.
    add_subdirectory("${dependencies_flatui_dir}" ${tmp_dir}/flatui)
  endif()

  if(NOT TARGET breadboard)
    # Include Breadboard event system.
    add_subdirectory("${dependencies_breadboard_dir}" ${tmp_dir}/breadboard)
  endif()

  if (NOT TARGET motive)
    # Include Motive animation system.
    set(motive_build_samples OFF CACHE BOOL "")
    set(motive_build_tests OFF CACHE BOOL "")
    add_subdirectory("${dependencies_motive_dir}" ${tmp_dir}/motive)
  endif()

  if(NOT TARGET pindrop)
    # Include Pindrop audio system.
    set(pindrop_build_demo OFF CACHE BOOL "")
    set(pindrop_build_tests OFF CACHE BOOL "")
    add_subdirectory("${dependencies_pindrop_dir}" ${tmp_dir}/pindrop)
  endif()

  if(NOT TARGET scene_lab)
    # Include Pindrop audio system.
    add_subdirectory("${dependencies_scene_lab_dir}" ${tmp_dir}/scene_lab)
  endif()

  include_directories(src include)
  get_property(FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR
    TARGET fplbase_generated_includes
    PROPERTY GENERATED_INCLUDES_DIR)
  include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
  get_property(CORGI_COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR
    TARGET corgi_component_library_generated_includes
    PROPERTY GENERATED_INCLUDES_DIR)
  include_directories(
    ${CORGI_COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR})
  get_property(SCENE_LAB_FLATBUFFERS_GENERATED_INCLUDES_DIR
    TARGET scene_lab_generated_includes
    PROPERTY GENERATED_INCLUDES_DIR)
  include_directories(
    ${SCENE_LAB_FLATBUFFERS_GENERATED_INCLUDES_DIR})
  include_directories(${dependencies_bulletphysics_distr_dir}/src)
  include_directories(${dependencies_corgi_dir}/include)
  include_directories(${dependencies_corgi_component_library_dir}/include)
  include_directories(${dependencies_breadboard_dir}/include)
  include_directories(${dependencies_flatbuffers_dir}/include)
  include_directories(${dependencies_fplbase_dir}/include)
  include_directories(${dependencies_fplutil_dir}/libfplutil/include)
  include_directories(${dependencies_flatui_dir}/include)
  include_directories(${dependencies_mathfu_dir}/include)
  include_directories(${dependencies_pindrop_dir}/include)
  include_directories(${dependencies_scene_lab_dir}/include)
  include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)
endif()

include_directories(${dependencies_motive_dir}/include)

set(scene_lab_sample_SRCS
    src/default_entity_factory.cpp
    src/game.cpp
    src/game.h
    src/main.cpp
    # For outputting flatbuffer files as json.
    ${dependencies_flatbuffers_dir}/src/idl_parser.cpp
    ${dependencies_flatbuffers_dir}/src/idl_gen_text.cpp
    )

if(scene_lab_build_cwebp)
  set(cwebp_option --cwebp $<TARGET_FILE:cwebp>)
  set(cwebp_depends cwebp)
endif()

add_custom_target(scene_lab_sample_assets
  COMMAND python scripts/build_assets.py
  --flatc $<TARGET_FILE:flatc>
  ${cwebp_option}
  --output ${CMAKE_SOURCE_DIR}/assets
  --copy_tree ${dependencies_flatui_dir}/assets ${CMAKE_SOURCE_DIR}/assets
  DEPENDS flatc ${cwebp_depends})

set(scene_lab_sample_flatbuffers_generated_includes_dir
   ${CMAKE_SOURCE_DIR}/${CMAKE_FILES_DIRECTORY}/include)

file(GLOB_RECURSE scene_lab_sample_flatbuffers_schemas
                  ${CMAKE_CURRENT_LIST_DIR}/schemas/*.fbs)

set(scene_lab_sample_schema_include_dirs
        ${CMAKE_CURRENT_LIST_DIR}/schemas
        ${dependencies_corgi_component_library_dir}/schemas
        ${dependencies_fplbase_dir}/schemas
        ${dependencies_motive_dir}/schemas
        ${dependencies_scene_lab_dir}/schemas
        ${dependencies_pindrop_dir}/schemas)

set(scene_lab_sample_flatbuffers_dependencies pindrop motive)

build_flatbuffers("${scene_lab_sample_flatbuffers_schemas}"
                  "${scene_lab_sample_schema_include_dirs}"
                  "scene_lab_sample_generated_includes"
                  "${scene_lab_sample_flatbuffers_dependencies}"
                  "${scene_lab_sample_flatbuffers_generated_includes_dir}"
                  "${CMAKE_SOURCE_DIR}/assets/flatbufferschemas"
                  "${CMAKE_SOURCE_DIR}/assets/flatbufferschemas")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

# Flatbuffers requires C++11.
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
endif()


set (SDL_LIBRARIES SDL2-static)
if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  set(OPENGL_LIBRARIES opengl32)
  set(SDL_LIBRARIES SDL2main ${SDL_LIBRARIES})
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  # C4127: conditional expression is constant
  # C4577: 'noexcept' used with no exception handling mode specified.
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4127 /wd4577")
  set(CMAKE_CXX_FLAGS "/MP")
  add_definitions(-DNOMINMAX -D_HAS_EXCEPTIONS=0 -D_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra")
endif()
if(APPLE)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers -Wno-overloaded-virtual")
  # These additional flags are needed because of Bullet's btSolverBody.h and
  # btConvexHullShape.h causing warnings with the Apple build
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

add_executable(scene_lab_sample ${scene_lab_sample_SRCS})
add_dependencies(scene_lab_sample scene_lab)
add_dependencies(scene_lab_sample scene_lab_sample_assets)
add_dependencies(scene_lab_sample scene_lab_sample_generated_includes)
mathfu_configure_flags(scene_lab_sample)
target_link_libraries(scene_lab_sample
  corgi_component_library
  motive
  fplbase
  scene_lab
  flatui
  flatbuffers
  breadboard
  corgi
  breadboard_module_library
  pindrop)
