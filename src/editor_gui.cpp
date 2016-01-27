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

#include "scene_lab/editor_gui.h"
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <vector>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "scene_lab/scene_lab.h"

namespace scene_lab {

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
                     fplbase::AssetManager* asset_manager,
                     fplbase::InputSystem* input_system,
                     fplbase::Renderer* renderer,
                     flatui::FontManager* font_manager)
    : config_(config),
      scene_lab_(scene_lab),
      asset_manager_(asset_manager),
      input_system_(input_system),
      renderer_(renderer),
      font_manager_(font_manager),
      edit_entity_(EntitySystemAdapter::kNoEntityId),
      changed_edit_entity_(EntitySystemAdapter::kNoEntityId),
      auto_commit_component_(EntitySystemAdapter::kNoComponentId),
      auto_revert_component_(EntitySystemAdapter::kNoComponentId),
      auto_recreate_component_(EntitySystemAdapter::kNoComponentId),
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
      [this](const GenericEntityId& entity) { EntityUpdated(entity); });

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

void EditorGui::EntityUpdated(const GenericEntityId& entity) {
  if (updated_via_gui_) return;  // Ignore this event if the GUI did the update.
  // If the entity we are looking at was updated externally, clear out its data.
  if (edit_entity_ == entity) {
    ClearEntityData();
  }
}

void EditorGui::SetEditEntity(const GenericEntityId& entity) {
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
  if (auto_commit_component_ != EntitySystemAdapter::kNoComponentId) {
    CommitComponentData(auto_commit_component_);
    auto_commit_component_ = EntitySystemAdapter::kNoComponentId;
    SendUpdateEvent();
  } else if (auto_revert_component_ != EntitySystemAdapter::kNoComponentId &&
             component_guis_.find(auto_revert_component_) !=
                 component_guis_.end()) {
    component_guis_.erase(auto_revert_component_);
    auto_revert_component_ = EntitySystemAdapter::kNoComponentId;
  } else if (auto_recreate_component_ != EntitySystemAdapter::kNoComponentId &&
             component_guis_.find(auto_recreate_component_) !=
                 component_guis_.end()) {
    // TODO: Implement recreating the entity. This requires you to
    // delete and recreate the entity, but with one component's data replaced.
    auto_recreate_component_ = EntitySystemAdapter::kNoComponentId;
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

void EditorGui::CommitComponentData(const GenericComponentId& id) {
  if (component_guis_.find(id) != component_guis_.end()) {
    FlatbufferEditor* editor = component_guis_[id].get();
    if (editor->flatbuffer_modified()) {
      entity_system_adapter()->DeserializeEntityComponent(
          edit_entity_, id,
          static_cast<const unsigned char*>(editor->flatbuffer()));
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
        entity_system_adapter()->RefreshPrototypeIDs();
      } else if (new_edit_view == kEntityList) {
        entity_system_adapter()->RefreshEntityIDs();
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
  if (edit_entity_ == EntitySystemAdapter::kNoEntityId) {
    flatui::Label("No entity selected!", config_->gui_button_size());
  } else {
    changed_edit_entity_ = EntitySystemAdapter::kNoEntityId;

    std::vector<GenericComponentId> components;
    entity_system_adapter()->GetEntityComponentList(edit_entity_, &components);
    for (auto i = components.begin(); i != components.end(); ++i) {
      DrawEntityComponent(*i);
    }
    DrawEntityFamily();

    if (changed_edit_entity_ != EntitySystemAdapter::kNoEntityId) {
      // Something during the course of rendering the UI caused the selected
      // entity to change, so let's select the new entity.
      SetEditEntity(changed_edit_entity_);
      changed_edit_entity_ = EntitySystemAdapter::kNoEntityId;
    }
  }
}

void EditorGui::DrawEntityListUI() {
  changed_edit_entity_ = EntitySystemAdapter::kNoEntityId;

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

  std::vector<GenericEntityId> entity_list;
  if (entity_system_adapter()->GetAllEntityIDs(&entity_list)) {
    for (auto e = entity_list.begin(); e != entity_list.end(); ++e) {
      const GenericEntityId& entity_id = *e;
      if (entity_system_adapter()->FilterShowEntityID(entity_id,
                                                      entity_list_filter_)) {
        EntityButton(entity_id, config_->gui_button_size());
      }
    }
  }

  if (changed_edit_entity_ != EntitySystemAdapter::kNoEntityId) {
    // select new entity
    if (changed_edit_entity_ == edit_entity_) {
      // select the same entity again, so change the tab mode
      edit_view_ = kEditEntity;
    }
    SetEditEntity(changed_edit_entity_);
    changed_edit_entity_ = EntitySystemAdapter::kNoEntityId;
  }
}

void EditorGui::DrawPrototypeListUI() {
  flatui::StartGroup(flatui::kLayoutHorizontalCenter, kSpacing,
                     "ws:prototype-list-filter");
  flatui::SetTextColor(text_normal_color_);
  flatui::Label("Filter:", config_->gui_button_size());
  vec2 size_vec = prototype_list_filter_.length() > 0
                      ? vec2(0, 0)
                      : vec2(kBlankEditWidth, 0);
  flatui::SetTextColor(text_editable_color_);
  if (flatui::Edit(config_->gui_button_size(), size_vec,
                   "ws:prototype-list-edit", nullptr,
                   &prototype_list_filter_)) {
    keyboard_in_use_ = true;
  }
  flatui::EndGroup();  // ws:prototype-list-filter

  const float kButtonSize = config_->gui_toolbar_size();

  std::vector<GenericEntityId> prototype_list;
  if (entity_system_adapter()->GetAllPrototypeIDs(&prototype_list)) {
    int i = 0;
    for (auto it = prototype_list.begin(); it != prototype_list.end(); ++it) {
      const GenericEntityId& prototype_id = *it;
      if (entity_system_adapter()->FilterShowEntityID(prototype_id,
                                                      prototype_list_filter_)) {
        std::string prototype_id_str;
        if (!entity_system_adapter()->GetEntityName(prototype_id,
                                                    &prototype_id_str)) {
          prototype_id_str = "prototype-" + std::to_string(i);
        }
        if (TextButton(prototype_id_str.c_str(),
                       (std::string("we:prototype-button-") + prototype_id_str)
                           .c_str(),
                       kButtonSize) &
            flatui::kEventWentUp) {
          GenericEntityId new_entity;
          if (entity_system_adapter()->CreateEntityFromPrototype(prototype_id,
                                                                 &new_entity)) {
            scene_lab_->MoveEntityToCamera(new_entity);
            SetEditEntity(new_entity);
            scene_lab_->SelectEntity(new_entity);
          }
        }
      }
    }
    ++i;
  }
}

void EditorGui::DrawEntityComponent(const GenericComponentId& id) {
  const float kTableNameSize = 30.0f;
  const float kTableButtonSize = kTableNameSize - 8.0f;

  if (component_guis_.find(id) == component_guis_.end()) {
    // No GUI editor found for this component, can we create one?
    flatbuffers::unique_ptr_t entity_data;
    if (entity_system_adapter()->SerializeEntityComponent(edit_entity_, id,
                                                          &entity_data)) {
      const reflection::Schema* schema = nullptr;
      const reflection::Object* obj = nullptr;
      entity_system_adapter()->GetSchema(&schema);
      entity_system_adapter()->GetTableObject(id, &obj);

      FlatbufferEditor* editor =
          new FlatbufferEditor(config_->flatbuffer_editor_config(), *schema,
                               *obj, entity_data.get());
      component_guis_[id].reset(editor);
    }
  }
  if (component_guis_.find(id) != component_guis_.end()) {
    std::string table_name;
    entity_system_adapter()->GetTableName(id, &table_name);

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
    bool from_proto = entity_system_adapter()->IsEntityComponentFromPrototype(
        edit_entity_, id);
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
  std::vector<GenericEntityId> children;
  GenericEntityId parent;
  if (entity_system_adapter()->GetEntityParent(edit_entity_, &parent)) {
    flatui::Label(" ", 20);  // insert some blank space
    if (parent != EntitySystemAdapter::kNoEntityId) {
      flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing, "we:parent");
      flatui::Label("Parent:", 24);
      EntityButton(parent, config_->gui_button_size());
      flatui::EndGroup();  // we:parent
    }
  }
  if (entity_system_adapter()->GetEntityChildren(edit_entity_, &children)) {
    flatui::Label(" ", 20);  // insert some blank space
    if (children.size() > 0) {
      flatui::StartGroup(flatui::kLayoutVerticalLeft, kSpacing, "we:children");
      flatui::Label("Children:", 24);
      for (auto it = children.begin(); it != children.end(); ++it) {
        EntityButton(*it, config_->gui_button_size());
      }
      flatui::EndGroup();  // we:children
    }
  }
}

void EditorGui::EntityButton(const GenericEntityId& entity_id, float size) {
  std::string entity_id_str;
  std::string entity_id_text;
  std::string entity_desc;
  if (entity_system_adapter()->GetEntityName(entity_id, &entity_id_str)) {
    entity_id_text = entity_id_str;
    if (entity_system_adapter()->GetEntityDescription(entity_id,
                                                      &entity_desc) &&
        entity_desc.length() > 0) {
      entity_id_text += "  (";
      entity_id_text += entity_desc;
      entity_id_text += ")";
    }
    auto event = TextButton(
        entity_id_text.c_str(),
        (std::string("we:entity-button-") + entity_id_str).c_str(), size);
    if (event & flatui::kEventWentUp) {
      changed_edit_entity_ = entity_id;
    }
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

EntitySystemAdapter* EditorGui::entity_system_adapter() {
  return scene_lab_->entity_system_adapter();
}

}  // namespace scene_lab
