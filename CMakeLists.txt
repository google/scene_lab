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

project(world_editor)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_CURRENT_BINARY_DIR}/obj)

# World Editor source files.
set(world_editor_SRCS
    include/world_editor/edit_options.h
    include/world_editor/editor_controller.h
    include/world_editor/editor_gui.h
    include/world_editor/flatbuffer_editor.h
    include/world_editor/world_editor.h
    src/edit_options.cpp
    src/editor_controller.cpp
    src/editor_gui.cpp
    src/flatbuffer_editor.cpp
    src/world_editor.cpp
 )

# Compile the game with the debug flag
set(world_editor_DEBUG ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
get_directory_property(world_editor_has_parent PARENT_DIRECTORY)

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
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
set(dependencies_entity_dir "${fpl_root}/entity"
    CACHE PATH "Directory containing the Entity library.")
set(dependencies_flatui_dir "${fpl_root}/flatui"
    CACHE PATH "Directory containing the FlatUI library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")
set(dependencies_bulletphysics_distr_dir "${third_party_root}/bulletphysics"
    CACHE PATH "Directory containing the Bullet Physics distribution.")
set(dependencies_component_library_dir "${dependencies_entity_dir}/component_library"
    CACHE PATH "Directory containing the entity system component library")

set(world_editor_standalone_mode OFF)
message(STATUS "CMAKE_SOURCE_DIR: " ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  set(world_editor_standalone_mode ON)
  message(STATUS "World Editor standalone: building with sample game")
else()
  message(STATUS "World Editor library: not building sample game")
endif()

option(world_editor_build_sample "Build a sample game for the world editor"
       ${world_editor_standalone_mode})

if(world_editor_standalone_mode)
  # Include FPLBase in this project.
  add_subdirectory("${dependencies_fplbase_dir}" ${tmp_dir}/fplbase)

  # Include FlatBuffers in this project.
  set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
  if(NOT TARGET flatc)
    add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)
  endif()

  # Include MathFu without tests or benchmarks.
  set(mathfu_build_benchmarks OFF CACHE BOOL "")
  set(mathfu_build_tests OFF CACHE BOOL "")
  add_subdirectory(${dependencies_mathfu_dir} ${tmp_dir}/mathfu)

  # Include entity system.
  add_subdirectory("${dependencies_entity_dir}" ${tmp_dir}/entity)
endif()

set(WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR
   ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_FILES_DIRECTORY}/include)
if(world_editor_has_parent)
  set(WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR
      ${WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR} PARENT_SCOPE)
endif()

file(GLOB_RECURSE WORLD_EDITOR_FLATBUFFERS_SCHEMAS schemas/*.fbs)

foreach(flatbuffers_schema ${WORLD_EDITOR_FLATBUFFERS_SCHEMAS})
  get_filename_component(filename ${flatbuffers_schema} NAME_WE)
  set(flatbuffers_include
    ${WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR}/${filename}_generated.h)
  file(COPY ${flatbuffers_schema}
       DESTINATION ${CMAKE_BINARY_DIR}/assets/flatbufferschemas/)
  add_custom_command(
    OUTPUT ${flatbuffers_include}
    COMMAND flatc --gen-includes --gen-mutable
        -o ${WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR}
        -I ${dependencies_fplbase_dir}/schemas
        -c ${flatbuffers_schema}
    DEPENDS flatc ${flatbuffers_schema})
  list(APPEND WORLD_EDITOR_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(world_editor_generated_includes
  DEPENDS ${WORLD_EDITOR_GENERATED_INCLUDES})

include_directories(src include include/world_editor)
include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${WORLD_EDITOR_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${dependencies_bulletphysics_distr_dir}/src)
include_directories(${dependencies_entity_dir}/include)
include_directories(${dependencies_component_library_dir}/include)
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${dependencies_fplbase_dir}/include)
include_directories(${dependencies_fplutil_dir}/libfplutil/include)
include_directories(${dependencies_flatui_dir}/include)
include_directories(${dependencies_motive_dir}/include)
include_directories(${dependencies_mathfu_dir}/include)

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

add_library(world_editor ${world_editor_SRCS})
add_dependencies(world_editor world_editor_generated_includes)
add_dependencies(world_editor component_library_generated_includes)
add_dependencies(world_editor fplbase_generated_includes)
mathfu_configure_flags(world_editor)


