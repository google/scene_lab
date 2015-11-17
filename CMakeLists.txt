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

project(scene_lab)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_CURRENT_BINARY_DIR}/obj)

# Scene Lab source files.
set(scene_lab_SRCS
    include/scene_lab/basic_camera.h
    include/scene_lab/edit_options.h
    include/scene_lab/editor_controller.h
    include/scene_lab/editor_gui.h
    include/scene_lab/flatbuffer_editor.h
    include/scene_lab/scene_lab.h
    include/scene_lab/util.h
    src/basic_camera.cpp
    src/edit_options.cpp
    src/editor_controller.cpp
    src/editor_gui.cpp
    src/flatbuffer_editor.cpp
    src/scene_lab.cpp
    src/util.cpp
    )

# Compile the game with the debug flag
set(scene_lab_DEBUG ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
get_directory_property(scene_lab_has_parent PARENT_DIRECTORY)

# Directory which contains the source for 3rd party libraries.
if(NOT DEFINED third_party_root)
  get_filename_component(
    third_party_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../../external"
    REALPATH)
endif()

# Directory which contains source for FPL libraries.
if(NOT DEFINED fpl_root)
  get_filename_component(
      fpl_root "${CMAKE_CURRENT_SOURCE_DIR}/../../libs" REALPATH)
endif()

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
if(NOT DEFINED dependencies_root)
  set(dependencies_root "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
  if(EXISTS "${dependencies_root}")
    set(third_party_root "${dependencies_root}")
    set(fpl_root "${dependencies_root}")
  endif()
endif()

set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
set(dependencies_breadboard_dir "${fpl_root}/breadboard"
    CACHE PATH "Directory containing the breadboard library.")
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
set(dependencies_corgi_dir "${fpl_root}/corgi"
    CACHE PATH "Directory containing the CORGI library.")
set(dependencies_flatui_dir "${fpl_root}/flatui"
    CACHE PATH "Directory containing the FlatUI library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_bulletphysics_distr_dir "${third_party_root}/bulletphysics"
    CACHE PATH "Directory containing the Bullet Physics distribution.")
set(dependencies_corgi_component_library_dir
    "${dependencies_corgi_dir}/component_library"
    CACHE PATH "Directory containing the CORGI component library")

set(scene_lab_standalone_mode OFF)
message(STATUS "CMAKE_SOURCE_DIR: " ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  set(scene_lab_standalone_mode ON)
  message(STATUS "Scene Lab standalone: building with sample game")
else()
  message(STATUS "Scene Lab library: not building sample game")
endif()

option(scene_lab_build_sample "Build a sample game for Scene Lab"
       ${scene_lab_standalone_mode})

option(scene_lab_build_cwebp "Build cwebp for Scene Lab from source." OFF)

if(scene_lab_standalone_mode)
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
endif()

set(scene_lab_flatbuffers_generated_includes_dir
   ${CMAKE_SOURCE_DIR}/${CMAKE_FILES_DIRECTORY}/include)

file(GLOB_RECURSE scene_lab_flatbuffers_schemas schemas/*.fbs)

set(scene_lab_schema_include_dirs
  ${CMAKE_SOURCE_DIR}/schemas
  ${dependencies_corgi_component_library_dir}/schemas
  ${dependencies_fplbase_dir}/schemas
  ${dependencies_motive_dir}/schemas
  ${dependencies_pindrop_dir}/schemas
  )

build_flatbuffers("${scene_lab_flatbuffers_schemas}"
                  "${scene_lab_schema_include_dirs}"
                  "scene_lab_generated_includes"
                  ""
                  "${scene_lab_flatbuffers_generated_includes_dir}"
                  ""
                  "")

include_directories(src include include/scene_lab)
get_property(FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET fplbase_generated_includes
             PROPERTY GENERATED_INCLUDES_DIR)
include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(CORGI_COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET corgi_component_library_generated_includes
             PROPERTY GENERATED_INCLUDES_DIR)
include_directories(
  ${CORGI_COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR})
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

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# Detect clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  # We need to ignore a bunch of different warnings because of the Bullet headers.
  # This will go away if we ever fix PhysicsComponent to stop exposing Bullet.
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4065 /wd4355 /wd4127 /wd4244 /wd4512 /wd4800")
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

add_library(scene_lab ${scene_lab_SRCS})
add_dependencies(scene_lab scene_lab_generated_includes)
add_dependencies(scene_lab corgi_component_library_generated_includes)
add_dependencies(scene_lab fplbase_generated_includes)
mathfu_configure_flags(scene_lab)

if(scene_lab_build_sample AND NOT TARGET scene_lab_sample)
  add_subdirectory(sample)
endif()

