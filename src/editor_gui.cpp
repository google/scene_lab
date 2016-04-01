// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <stdlib.h>
#include <string>
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/meta.h"
#include "corgi_component_library/transform.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "scene_lab/editor_gui.h"
#include "scene_lab/scene_lab.h"

namespace scene_lab {

using corgi::component_library::CommonServicesComponent;
using corgi::component_library::MetaComponent;
using corgi::component_library::MetaData;
using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;
using flatbuffers::Offset;
using flatbuffers::NumToString;
using flatbuffers::Table;
using flatbuffers::uoffset_t;
using flatbuffers::Vector;
using mathfu::vec2;

static const float kSpacing = 3.0f;
static const float kVirtualResolution = 1000.0f;
static const float kButtonMargin = 5.0f;
static const float kBlankEditWidth = 20.0f;

// Names of the mouse modes from SceneLab.cpp, in the same order as
// the mouse_mode_ enum. nullptr is an end sentinel to start over at 0.
static const char* const kMouseModeNames[] = {
    "Move Horizontally", "Move Vertically", "Rotate Horizontally",
    "Rotate Vertically", "Scale All",       "Scale X",
    "Scale Y",           "Scale Z",         nullptr};

EditorGui::EditorGui(const SceneLabConfig* config, SceneLab* scene_lab,
                     corgi::EntityManager* entity_manager,
                     flatui::FontManager* font_manager,
                     const std::string* schema_data)
    : config_(config),
      scene_lab_(scene_lab),
      entity_manager_(entity_manager),
      font_manager_(font_manager),
      schema_data_(schema_data),
      auto_commit_component_(0),
      auto_revert_component_(0),
      auto_recreate_component_(0),
      button_pressed_(kNone),
      edit_window_state_(kNormal),
      edit_view_(kEditEntity),
      edit_width_(0),
      mouse_mode_index_(0),
      show_physics_(false),
      show_types_(false),
      expand_all_(false),
      mouse_in_window_(false),
      keyboard_in_use_(false),
      prompting_for_exit_(false),
      updated_via_gui_(false) {
  auto services = entity_manager_->GetComponent<CommonServicesComponent>();
  asset_manager_ = services->asset_manager();
  entity_factory_ = services->entity_factory();
  input_system_ = services->input_system();
  renderer_ = services->renderer();
  components_to_show_.resize(entity_manager->ComponentCount(), false);
  components_to_show_[MetaComponent::GetComponentId()] = true;
  components_to_show_[TransformComponent::GetComponentId()] = true;
  for (int i = 0; i < kEditViewCount; i++) {
    scroll_offset_[i] = mathfu::kZeros2f;
  }

  const FlatbufferEditorConfig* fbconfig = config->flatbuffer_editor_config();
  bg_toolbar_color_ = LoadColorRGBA(config->gui_bg_toolbar_color());
  bg_edit_ui_color_ = LoadColorRGBA(config->gui_bg_edit_ui_color());
  bg_button_color_ = LoadColorRGBA(fbconfig->bg_button_color());
  bg_hover_color_ = LoadColorRGBA(fbconfig->bg_button_hover_color());
  bg_click_color_ = LoadColorRGBA(fbconfig->bg_button_click_color());

  text_button_color_ = LoadColorRGBA(fbconfig->text_button_color());
  text_normal_color_ = LoadColorRGBA(fbconfig->text_normal_color());
  text_disabled_color_ = LoadColorRGBA(fbconfig->text_disabled_color());
  text_editable_color_ = LoadColorRGBA(fbconfig->text_editable_color());
  text_modified_color_ = LoadColorRGBA(fbconfig->text_modified_color());
  text_error_color_ = LoadColorRGBA(fbconfig->text_error_color());

  lock_camera_height_ = config->camera_movement_parallel_to_ground();

  scene_lab_->AddOnUpdateEntityCallback(
      [this](const corgi::EntityRef& entity) { EntityUpdated(entity); });

  set_menu_title_string(scene_lab_->version());
}

void EditorGui::Activate() {
  prompting_for_exit_ = false;
  scene_lab_->set_entities_modified(false);
}

bool EditorGui::CanExit() {
  if (!CanDeselectEntity() || keyboard_in_use() ||
      scene_lab_->entities_modified() || prompting_for_exit_) {
    if (!prompting_for_exit_ && scene_lab_->entities_modified()) {
      prompting_for_exit_ = true;
    }
    return false;
  } else {
    // You're all clear, kid, let's blow this thing and go home.
    return true;
  }
}

void EditorGui::EntityUpdated(corgi::EntityRef entity) {
  if (updated_via_gui_) return;  // Ignore this event if the GUI did the update.
  // If the entity we are looking at was updated externally, clear out its data.
  if (edit_entity_ == entity) {
    ClearEntityData();
  }
}

void EditorGui::SetEditEntity(corgi::EntityRef& entity) {
  if (edit_entity_ != entity) {
    ClearEntityData();
    scroll_offset_[kEditEntity] = mathfu::kZeros2f;
    edit_entity_ = entity;
  }
}

void EditorGui::GetVirtualResolution(vec2* resolution_output) {
  assert(resolution_output != nullptr);
  // calculate virtual x/y
  vec2 window_size = vec2(renderer_->window_size());
  if (window_size.x() > window_size.y()) {
    float aspect = window_size.x() / window_size.y();
    *resolution_output = vec2(aspect * kVirtualResolution, kVirtualResolution);
  } else {
    float aspect = window_size.y() / window_size.x();
    *resolution_output = vec2(kVirtualResolution, aspect * kVirtualResolution);
  }
}

void EditorGui::Render() {
  StartRender();
  vec2 virtual_resolution;
  GetVirtualResolution(&virtual_resolution);
  flatui::Run(*asset_manager_, *font_manager_, *input_system_, [&]() {
    flatui::SetVirtualResolution(kVirtualResolution);
    if (config_->gui_font() != nullptr)
      flatui::SetTextFont(config_->gui_font()->c_str());
    DrawGui(virtual_resolution);
  });
  FinishRender();
}

void EditorGui::StartRender() {
  mouse_in_window_ = false;
  keyboard_in_use_ = false;
}

void EditorGui::FinishRender() {
  for (auto iter = component_guis_.begin(); iter != component_guis_.end();
       ++iter) {
    FlatbufferEditor* editor = iter->second.get();
    editor->Update();
    if (editor->keyboard_in_use()) keyboard_in_use_ = true;
  }
  if (auto_commit_component_ != 0) {
    CommitComponentData(auto_commit_component_);
    auto_commit_component_ = 0;
    SendUpdateEvent();
  } else if (auto_revert_component_ != 0 &&
             component_guis_.find(auto_revert_component_) !=
                 component_guis_.end()) {
    component_guis_.erase(auto_revert_component_);
    auto_revert_component_ = 0;
  } else if (auto_recreate_component_ != 0 &&
             component_guis_.find(auto_recreate_component_) !=
                 component_guis_.end()) {
    // Delete and recreate the entity, but with one component's data replaced.
    auto meta_component = entity_manager_->GetComponent<MetaComponent>();

    std::vector<corgi::ComponentInterface::RawDataUniquePtr> exported_data;
    std::vector<const void*> exported_pointers;  // Indexed by component ID.
    exported_pointers.resize(entity_factory_->max_component_id() + 1, nullptr);
    for (corgi::ComponentId component_id = 0;
         component_id <= entity_factory_->max_component_id(); component_id++) {
      const MetaData* meta_data =
          meta_component->GetComponentData(edit_entity_);
      if (component_id == auto_recreate_component_) {
        exported_pointers[component_id] =
            component_guis_[auto_recreate_component_]->flatbuffer();
      } else if (meta_data->components_from_prototype.find(component_id) ==
                 meta_data->components_from_prototype.end()) {
        corgi::ComponentInterface* component =
            entity_manager_->GetComponent(component_id);
        if (component != nullptr) {
          exported_data.push_back(component->ExportRawData(edit_entity_));
          exported_pointers[component_id] = exported_data.back().get();
        }
      }
    }
    std::string old_source_file =
        meta_component->GetComponentData(edit_entity_)->source_file;
    std::vector<uint8_t> entity_serialized;
    if (entity_factory_->CreateEntityDefinition(exported_pointers,
                                                &entity_serialized)) {
      std::vector<std::vector<uint8_t>> entity_defs;
      entity_defs.push_back(entity_serialized);
      std::vector<uint8_t> entity_list_def;
      if (entity_factory_->SerializeEntityList(entity_defs, &entity_list_def)) {
        // create a new copy of the entity...
        std::vector<corgi::EntityRef> entities_created;
        if (entity_factory_->LoadEntityListFromMemory(entity_list_def.data(),
                                                      entity_manager_,
                                                      &entities_created) > 0) {
          for (size_t i = 0; i < entities_created.size(); i++) {
            MetaData* meta_data = entity_manager_->GetComponentData<MetaData>(
                entities_created[i]);
            meta_data->source_file = old_source_file;
          }
          // delete the old entity...
          corgi::EntityRef old_entity = edit_entity_;
          SetEditEntity(entities_created[0]);
          entity_manager_->DeleteEntityImmediately(old_entity);
          entity_manager_->GetComponent<TransformComponent>()->PostLoadFixup();
        }
      }
    }
    auto_recreate_component_ = 0;
  }

  switch (button_pressed_) {
    case kNone: {
      break;
    }
    case kWindowMaximize: {
      edit_window_state_ = kMaximized;
      break;
    }
    case kWindowHide: {
      edit_view_ = kNoEditView;
      break;
    }
    case kWindowRestore: {
      edit_window_state_ = kNormal;
      break;
    }
    case kToggleDataTypes: {
      show_types_ = !show_types_;
      break;
    }
    case kToggleExpandAll: {
      expand_all_ = !expand_all_;
      break;
    }
    case kTogglePhysics: {
      show_physics_ = !show_physics_;
      break;
    }
    case kToggleLockCameraHeight: {
      lock_camera_height_ = !lock_camera_height_;
      break;
    }
    case kEntityCommit: {
      CommitEntityData();
      break;
    }
    case kEntityRevert: {
      ClearEntityData();
      break;
    }
  }
  button_pressed_ = kNone;
}

void EditorGui::DrawGui(const vec2& virtual_resolution) {
  virtual_resolution_ = virtual_resolution;

  if (edit_window_state_ == kMaximized)
    edit_width_ = virtual_resolution_.x();
  else if (edit_window_state_ == kNormal)
    edit_width_ = virtual_resolution_.x() / 3.0f;

  flatui::StartGroup(flatui::kLayoutOverlay, 0, "we:overall-ui");

  const float kButtonSize = config_->gui_toolbar_size();
  const float kTextSize = kButtonSize - 2.0f * kButtonMargin;

  // Show a bunch of buttons along the top of the screen.
  flatui::StartGroup(flatui::kLayoutHorizontalCenter, 10, "we:button-bg");
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignTop,
                        mathfu::kZeros2f);
  CaptureMouseClicks();
  flatui::ColorBackground(bg_toolbar_color_);
  flatui::SetMargin(flatui::Margin(virtual_resolution_.x(),
                                   config_->gui_toolbar_size(), 0, 0));
  flatui::EndGroup();  // button-bg

  flatui::StartGroup(flatui::kLayoutHorizontalCenter, 14, "we:buttons");
  flatui::PositionGroup(flatui::kAlignLeft, flatui::kAlignTop,
                        mathfu::kZeros2f);
  CaptureMouseClicks();
  flatui::Label(" ", kTextSize);  // Add a little spacing
  flatui::Label(menu_title_string().c_str(), kTextSize);

  if (TextButton("[Save Scene]", "we:save", kButtonSize) &
      flatui::kEventWentUp) {
    scene_lab_->SaveScene(true);
  }
  if (TextButton("[Exit Scene Lab]", "we:exit", kButtonSize) &
      flatui::kEventWentUp) {
    scene_lab_->RequestExit();
  }

  if (EntityModified()) {
    if (TextButton("[Revert All Changes]", "we:revert", kButtonSize) &
        flatui::kEventWentUp) {
      button_pressed_ = kEntityRevert;
    }
    if (TextButton("[Commit All Changes]", "we:commit", kButtonSize) &
        flatui::kEventWentUp) {
      button_pressed_ = kEntityCommit;
    }
  }
  flatui::EndGroup();  // we:buttons

  DrawTabs();
  if (edit_view_ != kNoEditView) {
    BeginDrawEditView();
    if (edit_view_ == kEditEntity) {
      DrawEditEntityUI();
    } else if (edit_view_ == kEntityList) {
      DrawEntityListUI();
    } else if (edit_view_ == kSettings) {
      DrawSettingsUI();
    } else if (edit_view_ == kPrototypeList) {
      DrawPrototypeListUI();
    }
    FinishDrawEditView();
  }
  flatui::StartGroup(flatui::kLayoutHorizontalCenter, 10, "we:tools");
  flatui::PositionGroup(flatui::kAlignLeft, flatui::kAlignBottom,
                        mathfu::kZeros2f);
  flatui::ColorBackground(bg_toolbar_color_);
  CaptureMouseClicks();
  if (TextButton(
          (std::string("Mouse Mode: ") + kMouseModeNames[mouse_mode_index_])
              .c_str(),
          "we:mouse_mode", kButtonSize) &
      flatui::kEventWentUp) {
    mouse_mode_index_++;
    if (kMouseModeNames[mouse_mode_index_] == nullptr) mouse_mode_index_ = 0;
  }
  flatui::EndGroup();  // we:tools

  if (prompting_for_exit_) {
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10, "we:exit-prompt");
    flatui::ColorBackground(bg_toolbar_color_);
    flatui::SetMargin(20);
    flatui::Label("Save changes before exiting Scene Lab?", kButtonSize);
    if (TextButton("Yes, save to disk", "we:save-to-disk", kButtonSize) &
        flatui::kEventWentUp) {
      scene_lab_->SaveScene(true);
      prompting_for_exit_ = false;
    } else if (TextButton("No, but keep my changes in memory",
                          "we:save-to-memory", kButtonSize) &
               flatui::kEventWentUp) {
      scene_lab_->SaveScene(false);
      prompting_for_exit_ = false;
    } else if (TextButton("Hold on, don't exit!", "we:dont-exit", kButtonSize) &
               flatui::kEventWentUp) {
      scene_lab_->AbortExit();
      prompting_for_exit_ = false;
    }
    flatui::EndGroup();  // we:exit-prompt
  }
  flatui::EndGroup();  // we:overall-ui
}

void EditorGui::CommitEntityData() {
  // Go through the components that were modified, and for each one, add it back
  // as raw data.
  for (auto iter = component_guis_.begin(); iter != component_guis_.end();
       ++iter) {
    CommitComponentData(iter->first);
  }

  // All components have been saved out.
  SendUpdateEvent();
}

void EditorGui::SendUpdateEvent() {
  updated_via_gui_ = true;
  scene_lab_->NotifyUpdateEntity(edit_entity_);
  updated_via_gui_ = false;
}

void EditorGui::CommitComponentData(corgi::ComponentId id) {
  corgi::ComponentInterface* component = entity_manager_->GetComponent(id);
  if (component_guis_.find(id) != component_guis_.end()) {
    FlatbufferEditor* editor = component_guis_[id].get();
    if (editor->flatbuffer_modified()) {
      component->AddFromRawData(
          edit_entity_, flatbuffers::GetAnyRoot(
                            static_cast<const uint8_t*>(editor->flatbuffer())));
      scene_lab_->set_entities_modified(true);
    }
    editor->ClearFlatbufferModifiedFlag();
  }
}

void EditorGui::CaptureMouseClicks() {
  auto event = flatui::CheckEvent();
  // Check for any event besides hover; if so, we'll take over mouse clicks.
  if (event & ~flatui::kEventHover) {
    mouse_in_window_ = true;
  }
}

void EditorGui::BeginDrawEditView() {
  flatui::StartGroup(flatui::kLayoutVerticalLeft, 0, "we:edit-ui-container");
  flatui::PositionGroup(flatui::kAlignLeft, flatui::kAlignTop,
                        vec2(virtual_resolution_.x() - edit_width_,
                             2 * config_->gui_toolbar_size()));
  flatui::StartScroll(vec2(edit_width_, virtual_resolution_.y() -
                                            2 * config_->gui_toolbar_size()),
                      &scroll_offset_[edit_view_]);
  flatui::ColorBackground(bg_edit_ui_color_);
  flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing, "we:edit-ui-v");
  CaptureMouseClicks();
  flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing);
  flatui::SetMargin(flatui::Margin(edit_width_, 1, 0, 0));
  flatui::EndGroup();
  flatui::StartGroup(flatui::kLayoutHorizontalTop, kSpacing, "we:edit-ui-h");
  CaptureMouseClicks();
  flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing);
  flatui::SetMargin(flatui::Margin(1, virtual_resolution_.y(), 0, 0));
  flatui::EndGroup();
  flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing,
                     "we:edit-ui-scroll");
  flatui::SetMargin(flatui::Margin(10, 10));
}

void EditorGui::FinishDrawEditView() {
  flatui::EndGroup();  // we:edit-ui-v
  flatui::EndGroup();  // we:edit-ui-h
  flatui::EndGroup();  // we:edit-ui-scroll
  flatui::EndScroll();
  flatui::EndGroup();  // we:edit-ui-container
}

static const char* const kEditViewNames[] = {
    "Edit Entity", "List Entities", "Edit Proto", "List Protos", "Settings"};

void EditorGui::DrawTabs() {
  float kTabSpacing = 4;
  float kGrowSelectedTab = 4;
  float kTabButtonSize = 12;

  int new_edit_view = edit_view_;

  flatui::StartGroup(flatui::kLayoutOverlay, 0, "we:toolbar-bg");
  flatui::PositionGroup(
      flatui::kAlignLeft, flatui::kAlignTop,
      vec2(virtual_resolution_.x() - edit_width_, config_->gui_toolbar_size()));
  CaptureMouseClicks();

  flatui::StartGroup(flatui::kLayoutHorizontalBottom, 0, "we:toolbar-fill");
  flatui::SetMargin(
      flatui::Margin(edit_width_, config_->gui_toolbar_size(), 0, 0));
  flatui::ColorBackground(bg_edit_ui_color_);
  flatui::EndGroup();  // we:toolbar-fill

  flatui::StartGroup(flatui::kLayoutHorizontalBottom, kTabSpacing,
                     "we:toolbar");
  // do some math to get the tabs spaced evenly
  float num_tabs = kEditViewCount;
  float width_each = edit_width_ / num_tabs - kTabSpacing - 1;
  for (int i = 0; i < kEditViewCount; i++) {
    const char* view = kEditViewNames[i];
    // container for the tab
    flatui::StartGroup(
        flatui::kLayoutOverlay, 0,
        (std::string("we:toolbar-tab-container-") + view).c_str());
    flatui::StartGroup(flatui::kLayoutHorizontalBottom, 0,
                       (std::string("we:toolbar-tab-overlay-") + view).c_str());
    float width_adjust = 0;
    float size_adjust = 0;
    if (edit_view_ == i) {
      flatui::ColorBackground(mathfu::kZeros4f);
      flatui::SetTextColor(text_button_color_);
      width_adjust = kTabSpacing;  // make selected tab slightly bigger
      size_adjust = kGrowSelectedTab;
    } else {
      flatui::ColorBackground(bg_button_color_);
      flatui::SetTextColor(text_normal_color_);
    }
    flatui::SetMargin(flatui::Margin(width_each + width_adjust,
                                     config_->gui_toolbar_size(), 0, 0));
    auto event = flatui::CheckEvent();
    if (event & flatui::kEventWentUp) {
      new_edit_view = i;
      if (new_edit_view == kPrototypeList) {
        RefreshPrototypeList();
      }
    }
    flatui::EndGroup();  // we:toolbar-tab-overlay-view
    flatui::StartGroup(flatui::kLayoutHorizontalBottom, 0,
                       (std::string("we:toolbar-tab-label-") + view).c_str());
    flatui::Label(view, kTabButtonSize + size_adjust);
    flatui::EndGroup();  // we:toolbar-tab-label-view
    flatui::EndGroup();  // we:toolbar-tab-container-view
  }

  flatui::EndGroup();  // we:toolbar
  flatui::EndGroup();  // we:toolbar-bg
  if (new_edit_view >= 0 && new_edit_view < kEditViewCount) {
    edit_view_ = static_cast<EditView>(new_edit_view);
  }
}

void EditorGui::DrawSettingsUI() {
  const float kButtonSize = 30;
  if (TextButton(show_types_ ? "[Data types: On]" : "[Data types: Off]",
                 "we:types", kButtonSize) &
      flatui::kEventWentUp)
    button_pressed_ = kToggleDataTypes;
  if (TextButton(show_physics_ ? "[Show physics: On]" : "[Show physics: Off]",
                 "we:physics", kButtonSize) &
      flatui::kEventWentUp)
    button_pressed_ = kTogglePhysics;
  if (TextButton(expand_all_ ? "[Expand all: On]" : "[Expand all: Off]",
                 "we:expand", kButtonSize) &
      flatui::kEventWentUp)
    button_pressed_ = kToggleExpandAll;
  if (TextButton(lock_camera_height_ ? "[Ground Parallel Camera: On]"
                                     : "[Ground Parallel Camera: Off]",
                 "we:lock-camera-height", kButtonSize) &
      flatui::kEventWentUp)
    button_pressed_ = kToggleLockCameraHeight;
  if (edit_window_state_ == kNormal) {
    if (TextButton("[Maximize View]", "we:maximize", kButtonSize) &
        flatui::kEventWentUp)
      button_pressed_ = kWindowMaximize;
  } else if (edit_window_state_ == kMaximized) {
    if (TextButton("[Restore View]", "we:restore", kButtonSize) &
        flatui::kEventWentUp)
      button_pressed_ = kWindowRestore;
  }
  if (TextButton("[Hide View]", "we:hide", kButtonSize) & flatui::kEventWentUp)
    button_pressed_ = kWindowHide;
}

void EditorGui::DrawEditEntityUI() {
  if (!edit_entity_) {
    flatui::Label("No entity selected!", config_->gui_button_size());
  } else {
    changed_edit_entity_ = corgi::EntityRef();

    for (corgi::ComponentId id = 0; id <= entity_factory_->max_component_id();
         id++) {
      DrawEntityComponent(id);
    }
    DrawEntityFamily();

    if (changed_edit_entity_) {
      // Something during the course of rendering the UI caused the selected
      // entity to change, so let's select the new entity.
      SetEditEntity(changed_edit_entity_);
      changed_edit_entity_ = corgi::EntityRef();
    }
  }
}

void EditorGui::DrawEntityListUI() {
  changed_edit_entity_ = corgi::EntityRef();

  flatui::StartGroup(flatui::kLayoutHorizontalCenter, kSpacing,
                     "ws:entity-list-filter");
  flatui::SetTextColor(text_normal_color_);
  flatui::Label("Filter:", config_->gui_button_size());
  vec2 size_vec =
      entity_list_filter_.length() > 0 ? vec2(0, 0) : vec2(kBlankEditWidth, 0);
  flatui::SetTextColor(text_editable_color_);
  if (flatui::Edit(config_->gui_button_size(), size_vec, "ws:entity-list-edit",
                   nullptr, &entity_list_filter_)) {
    keyboard_in_use_ = true;
  }
  flatui::EndGroup();  // ws:entity-list-filter

  for (auto e = entity_manager_->begin(); e != entity_manager_->end(); ++e) {
    MetaData* meta_data =
        entity_manager_->GetComponentData<MetaData>(e.ToReference());
    // TODO: Use regular expressions or globbing
    if (entity_list_filter_.length() == 0 ||
        (meta_data != nullptr &&
         (entity_manager_->GetComponent<MetaComponent>()
                  ->GetEntityID(e.ToReference())
                  .find(entity_list_filter_) != std::string::npos ||
          meta_data->prototype.find(entity_list_filter_) !=
              std::string::npos))) {
      EntityButton(e.ToReference(), config_->gui_button_size());
    }
  }
  if (changed_edit_entity_ != corgi::EntityRef()) {
    // select new entity
    if (changed_edit_entity_ == edit_entity_) {
      // select the same entity again, so change the tab mode
      edit_view_ = kEditEntity;
    }
    SetEditEntity(changed_edit_entity_);
    changed_edit_entity_ = corgi::EntityRef();
  }
}

void EditorGui::RefreshPrototypeList() {
  auto prototype_data = entity_factory_->prototype_data();
  prototype_list_.clear();
  for(auto it = prototype_data.begin(); it != prototype_data.end(); ++it) {
    prototype_list_.push_back(it->first);
  }
  std::sort(prototype_list_.begin(), prototype_list_.end());
}

void EditorGui::DrawPrototypeListUI() {
  flatui::StartGroup(flatui::kLayoutHorizontalCenter, kSpacing,
                     "ws:prototype-list-filter");
  flatui::SetTextColor(text_normal_color_);
  flatui::Label("Filter:", config_->gui_button_size());
  vec2 size_vec = prototype_list_filter_.length() > 0 ?
      vec2(0, 0) : vec2(kBlankEditWidth, 0);
  flatui::SetTextColor(text_editable_color_);
  if (flatui::Edit(config_->gui_button_size(), size_vec,
                   "ws:prototype-list-edit", nullptr,
                   &prototype_list_filter_)) {
    keyboard_in_use_ = true;
  }
  flatui::EndGroup();  // ws:prototype-list-filter

  const float kButtonSize = config_->gui_toolbar_size();
  for (auto it = prototype_list_.begin(); it != prototype_list_.end(); ++it) {
    if (prototype_list_filter_.length() == 0 ||
        it->find(prototype_list_filter_) != std::string::npos) {
      if (TextButton(it->c_str(), ("we:prototype-button-" + (*it)).c_str(),
                     kButtonSize) & flatui::kEventWentUp){
        corgi::EntityRef new_entity = entity_factory_
            ->CreateEntityFromPrototype(it->c_str(), entity_manager_);
        entity_manager_->GetComponent<EditOptionsComponent>()
            ->EntityCreated(new_entity);
        scene_lab_->MoveEntityToCamera(new_entity);
        SetEditEntity(new_entity);
        scene_lab_->SelectEntity(new_entity);
      }
    }
  }
}

void EditorGui::DrawEntityComponent(corgi::ComponentId id) {
  const float kTableNameSize = 30.0f;
  const float kTableButtonSize = kTableNameSize - 8.0f;

  // Check if we have a FlatbufferEditor for this component.
  corgi::ComponentInterface* component = entity_manager_->GetComponent(id);
  if (component != nullptr &&
      component->GetComponentDataAsVoid(edit_entity_) != nullptr) {
    std::string table_name = entity_factory_->ComponentIdToTableName(id);
    if (component_guis_.find(id) == component_guis_.end()) {
      auto services = entity_manager_->GetComponent<CommonServicesComponent>();
      bool prev_force_defaults = services->export_force_defaults();
      // Force all default fields to have values that the user can edit
      services->set_export_force_defaults(true);

      auto raw_data = component->ExportRawData(edit_entity_);
      services->set_export_force_defaults(prev_force_defaults);

      const reflection::Schema* schema =
          reflection::GetSchema(schema_data_->c_str());
      const reflection::Object* obj =
          schema->objects()->LookupByKey(table_name.c_str());
      FlatbufferEditor* editor = new FlatbufferEditor(
          config_->flatbuffer_editor_config(), *schema, *obj, raw_data.get());
      component_guis_[id].reset(editor);
    }

    bool has_data = component_guis_[id]->HasFlatbufferData();

    flatui::StartGroup(flatui::kLayoutHorizontalBottom, kSpacing,
                       (table_name + "-container").c_str());
    flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing,
                       (table_name + "-title").c_str());
    if (component_guis_[id]->flatbuffer_modified()) {
      flatui::SetTextColor(text_modified_color_);
    } else {
      flatui::SetTextColor(text_normal_color_);
    }
    if (has_data) {
      auto event = flatui::CheckEvent();
      if (event & flatui::kEventWentDown) {
        components_to_show_[id] = !components_to_show_[id];
      }
      if (event & flatui::kEventHover) {
        flatui::ColorBackground(bg_hover_color_);
      }
    } else {
      // gray out the table name since we can't click it
      flatui::SetTextColor(text_disabled_color_);
    }
    flatui::Label(table_name.c_str(), kTableNameSize);
    flatui::SetTextColor(text_normal_color_);
    flatui::EndGroup();  // $table_name-title
    MetaData* meta_data =
        entity_manager_->GetComponentData<MetaData>(edit_entity_);
    bool from_proto = meta_data
                          ? (meta_data->components_from_prototype.find(id) !=
                             meta_data->components_from_prototype.end())
                          : false;
    if (component_guis_[id]->flatbuffer_modified()) {
      if (TextButton("[Commit]", (table_name + "-commit-to-entity").c_str(),
                     kTableButtonSize) &
          flatui::kEventWentUp) {
        auto_commit_component_ = id;
      }
      if (TextButton("[Revert]", (table_name + "-revert-entity").c_str(),
                     kTableButtonSize) &
          flatui::kEventWentUp) {
        auto_revert_component_ = id;
      }
      if (TextButton("[Reload]", (table_name + "-reload-entity").c_str(),
                     kTableButtonSize) &
          flatui::kEventWentUp) {
        auto_recreate_component_ = id;
      }
    } else if (has_data) {
      // Draw the title of the component table.
      flatui::Label(from_proto ? "(from prototype)" : "(from entity)", 12);
    } else {
      // Component is present but wasn't exported by ExportRawData.
      // Usually that means it was automatically generated and doesn't
      // need to be edited by a human.
      flatui::SetTextColor(text_disabled_color_);
      flatui::Label("(not exported)", 12);
      flatui::SetTextColor(text_normal_color_);
    }
    flatui::EndGroup();  // $table_name-container

    if (has_data && (expand_all_ || components_to_show_[id])) {
      flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing,
                         (table_name + "-contents").c_str());
      // Draw the actual edit controls for this component data.
      component_guis_[id]->set_show_types(show_types_);
      component_guis_[id]->set_expand_all(expand_all_);

      component_guis_[id]->Draw();
      flatui::EndGroup();  // $table_name-contents
    }
  }
}

void EditorGui::DrawEntityFamily() {
  // Show a list of this entity's parents and children.
  TransformData* transform_data =
      entity_manager_->GetComponentData<TransformData>(edit_entity_);
  if (transform_data != nullptr) {
    flatui::Label(" ", 20);  // insert some blank space
    if (transform_data->parent) {
      flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing, "we:parent");
      flatui::Label("Parent:", 24);
      EntityButton(transform_data->parent, config_->gui_button_size());
      flatui::EndGroup();  // we:parent
    }
    bool has_child = false;
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      corgi::EntityRef& child = iter->owner;
      if (!has_child) {
        has_child = true;
        flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing,
                           "we:children");
        flatui::Label("Children:", 24);
      }
      EntityButton(child, config_->gui_button_size());
    }
    if (has_child) {
      flatui::EndGroup();  // we:children
    }
  }
}

void EditorGui::EntityButton(const corgi::EntityRef& entity, float size) {
  MetaData* meta_data = entity_manager_->GetComponentData<MetaData>(entity);
  std::string entity_id =
      meta_data
          ? entity_manager_->GetComponent<MetaComponent>()->GetEntityID(
                const_cast<corgi::EntityRef&>(entity))
          : "Unknown entity ID";
  if (meta_data) {
    entity_id += "  (";
    entity_id += meta_data->prototype;
    entity_id += ")";
  }
  auto event =
      TextButton(entity_id.c_str(),
                 (std::string("we:entity-button-") + entity_id).c_str(), size);
  if (event & flatui::kEventWentUp) {
    changed_edit_entity_ = entity;
  }
}

flatui::Event EditorGui::TextButton(const char* text, const char* id,
                                    float size) {
  float text_size = size - 2 * kButtonMargin;
  flatui::StartGroup(flatui::kLayoutHorizontalTop, size / 4.0f, id);
  flatui::SetMargin(flatui::Margin(kButtonMargin));
  auto event = flatui::CheckEvent();
  if (event & ~flatui::kEventHover) {
    mouse_in_window_ = true;
    flatui::ColorBackground(bg_click_color_);
  } else if (event & flatui::kEventHover) {
    flatui::ColorBackground(bg_hover_color_);
  } else {
    flatui::ColorBackground(bg_button_color_);
  }
  flatui::SetTextColor(text_normal_color_);
  flatui::Label(text, text_size);
  flatui::EndGroup();  // $id
  return event;
}

}  // namespace scene_lab
