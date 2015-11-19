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

LOCAL_PATH:=$(call my-dir)/..

SCENE_LAB_SAMPLE_DIR:=$(LOCAL_PATH)
FLATBUFFERS_FLATC_ARGS:=--gen-mutable
include $(SCENE_LAB_SAMPLE_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

# realpath-portable From flatbuffers/android/jni/include.mk
LOCAL_PATH:=$(call realpath-portable,$(LOCAL_PATH))
SCENE_LAB_SAMPLE_DIR:=$(LOCAL_DIR)

# Build rule which builds assets for the game.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
.PHONY: build_assets
# Create a binary schema file for the components.fbs schema.
build_assets: $(flatc_target)
	-mkdir -p $(SCENE_LAB_SAMPLE_DIR)/assets
	$(hide) python $(SCENE_LAB_SAMPLE_DIR)/scripts/build_assets.py \
       --flatc $(FLATBUFFERS_FLATC) \
       --copy_tree $(DEPENDENCIES_FLATUI_DIR)/assets/shaders \
                   $(SCENE_LAB_SAMPLE_DIR)/assets/shaders \
                   $(DEPENDENCIES_FLATUI_DIR)/assets/fonts \
                   $(SCENE_LAB_SAMPLE_DIR)/assets/fonts
	-mkdir -p $(SCENE_LAB_SAMPLE_DIR)/assets/flatbufferschemas
        # Make a FlatBuffers binary schema for the components schema.
	$(FLATBUFFERS_FLATC) -b --schema \
	  $(foreach include,$(SCENE_LAB_SAMPLE_FLATBUFFER_INCLUDE_DIRS),-I $(include)) \
	  -o $(SCENE_LAB_SAMPLE_DIR)/assets/flatbufferschemas \
	  $(SCENE_LAB_SAMPLE_SCHEMA_DIR)/components.fbs



.PHONY: clean_assets
clean_assets:
	$(hide) python $(SCENE_LAB_SAMPLE_DIR)/scripts/build_assets.py clean
endif
PROJECT_GLOBAL_BUILD_RULES_DEFINED:=1
include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_ARM_MODE := arm

SCENE_LAB_GENERATED_OUTPUT_DIR := $(SCENE_LAB_DIR)/gen/include

LOCAL_C_INCLUDES := \
  $(DEPENDENCIES_CORGI_DIR)/include \
  $(DEPENDENCIES_CORGI_COMPONENT_LIBRARY_DIR)/include \
  $(DEPENDENCIES_BREADBOARD_DIR)/include \
  $(DEPENDENCIES_BREADBOARD_MODULE_LIBRARY_DIR)/include \
  $(DEPENDENCIES_SCENE_LAB_DIR)/include \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_GPG_DIR)/include \
  $(DEPENDENCIES_WEBP_DIR)/src \
  $(DEPENDENCIES_BULLETPHYSICS_DIR)/src \
  $(DEPENDENCIES_SCENE_LAB_DIR)/src \
  $(COMPONENTS_GENERATED_OUTPUT_DIR) \
  $(BREADBOARD_MODULE_LIBRARY_GENERATED_OUTPUT_DIR) \
  $(SCENE_LAB_GENERATED_OUTPUT_DIR) \
  $(SCENE_LAB_SAMPLE_GENERATED_OUTPUT_DIR) \
  src

LOCAL_SRC_FILES := \
  src/default_entity_factory.cpp \
  src/game.cpp \
  src/main.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/idl_parser.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/idl_gen_text.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/reflection.cpp

SCENE_LAB_SAMPLE_SCHEMA_DIR := $(SCENE_LAB_SAMPLE_DIR)/schemas

SCENE_LAB_SAMPLE_SCHEMA_FILES := \
  $(SCENE_LAB_SAMPLE_SCHEMA_DIR)/components.fbs

$(foreach src,$(LOCAL_SRC_FILES),\
  $(eval $(call local-source-file-path,$(src)): | build_assets))

SCENE_LAB_SAMPLE_FLATBUFFER_INCLUDE_DIRS := \
  $(SCENE_LAB_SAMPLE_SCHEMA_DIR) \
  $(DEPENDENCIES_SCENE_LAB_DIR)/schemas \
  $(DEPENDENCIES_PINDROP_DIR)/schemas \
  $(DEPENDENCIES_MOTIVE_DIR)/schemas \
  $(DEPENDENCIES_FPLBASE_DIR)/schemas \
  $(DEPENDENCIES_CORGI_COMPONENT_LIBRARY_DIR)/schemas \
  $(DEPENDENCIES_BREADBOARD_MODULE_LIBRARY_DIR)/schemas

# Override JNI_OnLoad functions.
FPLBASE_JNI_ONLOAD_FUNCTIONS := SDL_JNI_OnLoad

ifeq (,$(SCENE_LAB_SAMPLE_RUN_ONCE))
SCENE_LAB_SAMPLE_RUN_ONCE := 1
$(call flatbuffers_header_build_rules,\
  $(SCENE_LAB_SAMPLE_SCHEMA_FILES),\
  $(SCENE_LAB_SAMPLE_SCHEMA_DIR),\
  $(SCENE_LAB_SAMPLE_GENERATED_OUTPUT_DIR),\
  $(SCENE_LAB_SAMPLE_FLATBUFFER_INCLUDE_DIRS),\
  $(LOCAL_SRC_FILES),\
  scene_lab_sample_generated_includes,\
  corgi_component_library_generated_includes \
  breadboard_module_library_generated_includes \
  motive_generated_includes \
  fplbase_generated_includes \
  pindrop_generated_includes \
  scene_lab_generated_includes)

.PHONY: clean_generated_includes
clean_generated_includes:
	$(hide) rm -rf $(SCENE_LAB_SAMPLE_GENERATED_OUTPUT_DIR)
endif

.PHONY: clean
clean: clean_assets clean_generated_includes

LOCAL_STATIC_LIBRARIES := \
  libbreadboard \
  libmodule_library \
  libgpg \
  libmathfu \
  libwebp \
  libfplbase \
  libflatui \
  libpindrop \
  libcorgi \
  libcorgi_component_library \
  libscene_lab \
  libmotive \
  libfreetype \
  libharfbuzz \
  libflatbuffers \
  libbullet

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(DEPENDENCIES_BREADBOARD_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FPLBASE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATUI_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MOTIVE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_SCENE_LAB_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_PINDROP_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_CORGI_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_WEBP_DIR)/..)

$(call import-module,fplbase/jni)
$(call import-module,corgi/jni)
$(call import-module,breadboard/jni)
$(call import-module,breadboard/module_library/jni)
$(call import-module,pindrop/jni)
$(call import-module,flatbuffers/android/jni)
$(call import-module,flatui/jni)
$(call import-module,mathfu/jni)
$(call import-module,motive/jni)
$(call import-module,corgi/component_library/jni)
$(call import-module,scene_lab/jni)
$(call import-module,webp)

