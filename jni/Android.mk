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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := world_editor
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := libfplbase libbullet libentity libcomponent_library
LOCAL_SHARED_LIBRARIES :=

WORLD_EDITOR_RELATIVE_DIR := ..
WORLD_EDITOR_DIR := $(LOCAL_PATH)/$(WORLD_EDITOR_RELATIVE_DIR)

include $(WORLD_EDITOR_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

LOCAL_EXPORT_C_INCLUDES := \
  $(WORLD_EDITOR_DIR)/include \
  $(WORLD_EDITOR_GENERATED_OUTPUT_DIR)

LOCAL_C_INCLUDES := \
  $(LOCAL_EXPORT_C_INCLUDES) \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/include \
  $(DEPENDENCIES_FLATUI_DIR)/include \
  $(DEPENDENCIES_ENTITY_DIR)/include \
  $(DEPENDENCIES_COMPONENT_LIBRARY_DIR)/include \
  $(DEPENDENCIES_EVENT_DIR)/include \
  $(DEPENDENCIES_FPLBASE_DIR)/include \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_MATHFU_DIR)/include \
  $(DEPENDENCIES_BULLETPHYSICS_DIR)/src \
  $(WORLD_EDITOR_DIR)/src

LOCAL_SRC_FILES := \
  $(WORLD_EDITOR_RELATIVE_DIR)/src/world_editor.cpp \
  $(WORLD_EDITOR_RELATIVE_DIR)/src/edit_options.cpp

WORLD_EDITOR_SCHEMA_DIR := $(WORLD_EDITOR_DIR)/schemas
WORLD_EDITOR_SCHEMA_INCLUDE_DIRS := \
  $(DEPENDENCIES_FPLBASE_DIR)/schemas \
  $(DEPENDENCIES_COMPONENT_LIBRARY_DIR)/schemas

WORLD_EDITOR_SCHEMA_FILES := \
  $(WORLD_EDITOR_SCHEMA_DIR)/editor_components.fbs \
  $(WORLD_EDITOR_SCHEMA_DIR)/editor_events.fbs \
  $(WORLD_EDITOR_SCHEMA_DIR)/world_editor_config.fbs

ifeq (,$(WORLD_EDITOR_RUN_ONCE))
WORLD_EDITOR_RUN_ONCE := 1
$(call flatbuffers_header_build_rules, \
  $(WORLD_EDITOR_SCHEMA_FILES), \
  $(WORLD_EDITOR_SCHEMA_DIR), \
  $(WORLD_EDITOR_GENERATED_OUTPUT_DIR), \
  $(WORLD_EDITOR_SCHEMA_INCLUDE_DIRS), \
  $(LOCAL_SRC_FILES))
endif

include $(BUILD_STATIC_LIBRARY)

$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FPLBASE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_ENTITY_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATUI_DIR)/..)

$(call import-module,flatbuffers/android/jni)
$(call import-module,fplbase/jni)
$(call import-module,mathfu/jni)
$(call import-module,entity/jni)
$(call import-module,entity/component_library/jni)
$(call import-module,flatui/jni)
$(call import-module,android/cpufeatures)
