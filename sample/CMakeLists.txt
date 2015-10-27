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

project(scene_lab_sample)

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
  --output ${CMAKE_CURRENT_SOURCE_DIR}/assets
  DEPENDS flatc ${cwebp_depends})

# Copy FlatUI required assets
add_custom_command(TARGET scene_lab_sample_assets POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${dependencies_flatui_dir}/assets
                       ${CMAKE_CURRENT_SOURCE_DIR}/assets)

set(scene_lab_sample_flatbuffers_generated_includes_dir
   ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_FILES_DIRECTORY}/include)

file(GLOB_RECURSE scene_lab_sample_flatbuffers_schemas
                  ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.fbs)

set(scene_lab_sample_schema_include_dirs
        ${CMAKE_SOURCE_DIR}/schemas
        ${dependencies_component_library_dir}/schemas
        ${dependencies_fplbase_dir}/schemas
        ${dependencies_motive_dir}/schemas
        ${dependencies_pindrop_dir}/schemas)

set(scene_lab_sample_flatbuffers_dependencies pindrop motive)

build_flatbuffers("${scene_lab_sample_flatbuffers_schemas}"
                  "${scene_lab_sample_schema_include_dirs}"
                  "scene_lab_sample_generated_includes"
                  "${scene_lab_sample_flatbuffers_dependencies}"
                  "${scene_lab_sample_flatbuffers_generated_includes_dir}"
                  "${CMAKE_CURRENT_SOURCE_DIR}/assets/flatbufferschemas"
                  "${CMAKE_CURRENT_SOURCE_DIR}/assets/flatbufferschemas")

include_directories(${dependencies_motive_dir}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

add_executable(scene_lab_sample ${scene_lab_sample_SRCS})
add_dependencies(scene_lab_sample scene_lab)
add_dependencies(scene_lab_sample scene_lab_sample_assets)
add_dependencies(scene_lab_sample scene_lab_sample_generated_includes)
mathfu_configure_flags(scene_lab_sample)
target_link_libraries(scene_lab_sample
  component_library
  motive
  fplbase
  scene_lab
  flatui
  flatbuffers
  breadboard
  entity
  module_library
  pindrop)
