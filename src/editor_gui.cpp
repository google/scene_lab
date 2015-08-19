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

#include <stdlib.h>
#include <string>
#include "component_library/common_services.h"
#include "component_library/meta.h"
#include "component_library/transform.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "world_editor/editor_event.h"
#include "world_editor/editor_gui.h"

namespace fpl {
namespace editor {

using component_library::CommonServicesComponent;
using component_library::MetaComponent;
using component_library::MetaData;
using component_library::TransformComponent;
using component_library::TransformData;
using flatbuffers::Offset;
using flatbuffers::NumToString;
using flatbuffers::Table;
using flatbuffers::uoffset_t;
using flatbuffers::Vector;

EditorGui::EditorGui(const WorldEditorConfig* config,
                     entity::EntityManager* entity_manager,
                     FontManager* font_manager, const std::string* schema_data)
    : config_(config),
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
      show_physics_(false),
      show_types_(false),
      expand_all_(false),
      mouse_in_window_(false),
      keyboard_in_use_(false) {
  auto services = entity_manager_->GetComponent<CommonServicesComponent>();
  asset_manager_ = services->asset_manager();
  entity_factory_ = services->entity_factory();
  event_manager_ = services->event_manager();
  input_system_ = services->input_system();
  renderer_ = services->renderer();
  components_to_show_.resize(entity::kMaxComponentCount, false);
  components_to_show_[MetaComponent::GetComponentId()] = true;
  components_to_show_[TransformComponent::GetComponentId()] = true;
  for (int i = 0; i < kEditViewCount; i++) {
    scroll_offset_[i] = mathfu::kZeros2i;
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

  entity_manager_->GetComponent<component_library::CommonServicesComponent>()
      ->event_manager()
      ->RegisterListener(kEventSinkUnion_EditorEvent, this);
}

void EditorGui::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case kEventSinkUnion_EditorEvent: {
      // If the entity we are looking at was updated externally, clear out its
      // data.
      auto* editor_event = event_payload.ToData<editor::EditorEventPayload>();
      if (editor_event->action == EditorEventAction_EntityUpdated &&
          edit_entity_ == editor_event->entity) {
        ClearEntityData();
      }
      break;
    }
    default: { assert(false); }
  }
}

void EditorGui::SetEditEntity(entity::EntityRef& entity) {
  if (edit_entity_ != entity) {
    LogInfo("SetEditEntity()");
    ClearEntityData();
    scroll_offset_[kEditEntity] = mathfu::kZeros2i;
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
  mathfu::vec2 virtual_resolution;
  GetVirtualResolution(&virtual_resolution);
  gui::Run(*asset_manager_, *font_manager_, *input_system_, [&]() {
    gui::SetVirtualResolution(kVirtualResolution);
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

    std::vector<entity::ComponentInterface::RawDataUniquePtr> exported_data;
    std::vector<const void*> exported_pointers;  // Indexed by component ID.
    exported_pointers.resize(entity_factory_->max_component_id() + 1, nullptr);
    for (entity::ComponentId component_id = 0;
         component_id <= entity_factory_->max_component_id(); component_id++) {
      const MetaData* meta_data =
          meta_component->GetComponentData(edit_entity_);
      if (component_id == auto_recreate_component_) {
        exported_pointers[component_id] =
            component_guis_[auto_recreate_component_]->flatbuffer();
      } else if (meta_data->components_from_prototype.find(component_id) ==
                 meta_data->components_from_prototype.end()) {
        entity::ComponentInterface* component =
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
      std::vector<std::vector<uint8_t>> entity_defs = {entity_serialized};
      std::vector<uint8_t> entity_list_def;
      if (entity_factory_->SerializeEntityList(entity_defs, &entity_list_def)) {
        // create a new copy of the entity...
        std::vector<entity::EntityRef> entities_created;
        if (entity_factory_->LoadEntityListFromMemory(entity_list_def.data(),
                                                      entity_manager_,
                                                      &entities_created) > 0) {
          for (size_t i = 0; i < entities_created.size(); i++) {
            MetaData* meta_data = entity_manager_->GetComponentData<MetaData>(
                entities_created[i]);
            meta_data->source_file = old_source_file;
          }
          // delete the old entity...
          entity::EntityRef old_entity = edit_entity_;
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

void EditorGui::DrawGui(const mathfu::vec2& virtual_resolution) {
  virtual_resolution_ = virtual_resolution;

  if (edit_window_state_ == kMaximized)
    edit_width_ = virtual_resolution_.x();
  else if (edit_window_state_ == kNormal)
    edit_width_ = virtual_resolution_.x() / 3.0f;

  gui::StartGroup(gui::kLayoutOverlay, 0, "we:overall-ui");

  const float kButtonSize = config_->gui_toolbar_size();

  // Show a bunch of buttons along the top of the screen.
  gui::StartGroup(gui::kLayoutHorizontalCenter, 10, "we:button-bg");
  gui::PositionGroup(gui::kAlignCenter, gui::kAlignTop, mathfu::kZeros2f);
  CaptureMouseClicks();
  gui::ColorBackground(bg_toolbar_color_);
  gui::SetMargin(
      gui::Margin(virtual_resolution_.x(), config_->gui_toolbar_size(), 0, 0));
  gui::EndGroup();  // button-bg

  gui::StartGroup(gui::kLayoutHorizontalCenter, 14, "we:buttons");
  gui::PositionGroup(gui::kAlignLeft, gui::kAlignTop, mathfu::kZeros2f);
  CaptureMouseClicks();

  gui::Label(" World Editor", config_->gui_toolbar_size() - 8);

  TextButton("[Save World]", "we:save", config_->gui_toolbar_size());
  TextButton("[Exit Editor]", "we:exit", config_->gui_toolbar_size());

  if (EntityModified()) {
    if (TextButton("[Revert All Changes]", "we:revert", kButtonSize) &
        gui::kEventWentUp) {
      button_pressed_ = kEntityRevert;
    }
    if (TextButton("[Commit All Changes]", "we:commit", kButtonSize) &
        gui::kEventWentUp) {
      button_pressed_ = kEntityCommit;
    }
  }
  gui::EndGroup();  // we:buttons

  DrawTabs();
  if (edit_view_ != kNoEditView) {
    BeginDrawEditView();
    if (edit_view_ == kEditEntity) {
      DrawEditEntityUI();
    } else if (edit_view_ == kEntityList) {
      DrawEntityListUI();
    } else if (edit_view_ == kSettings) {
      DrawSettingsUI();
    }
    FinishDrawEditView();
  }
  gui::EndGroup();  // we:overall-ui
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
  event_manager_->BroadcastEvent(
      EditorEventPayload(EditorEventAction_EntityUpdated, edit_entity_));
}

void EditorGui::CommitComponentData(entity::ComponentId id) {
  entity::ComponentInterface* component = entity_manager_->GetComponent(id);
  if (component_guis_.find(id) != component_guis_.end()) {
    FlatbufferEditor* editor = component_guis_[id].get();
    if (editor->flatbuffer_modified()) {
      component->AddFromRawData(
          edit_entity_, flatbuffers::GetAnyRoot(
                            static_cast<const uint8_t*>(editor->flatbuffer())));
    }
    editor->ClearFlatbufferModifiedFlag();
  }
}

void EditorGui::CaptureMouseClicks() {
  auto event = gui::CheckEvent();
  // Check for any event besides hover; if so, we'll take over mouse clicks.
  if (event & ~gui::kEventHover) {
    mouse_in_window_ = true;
  }
}

void EditorGui::BeginDrawEditView() {
  gui::StartGroup(gui::kLayoutVerticalLeft, 0, "we:edit-ui-container");
  gui::PositionGroup(gui::kAlignLeft, gui::kAlignTop,
                     vec2(virtual_resolution_.x() - edit_width_,
                          2 * config_->gui_toolbar_size()));
  gui::StartScroll(vec2(edit_width_, virtual_resolution_.y() -
                                         2 * config_->gui_toolbar_size()),
                   &scroll_offset_[edit_view_]);
  CaptureMouseClicks();
  gui::ColorBackground(bg_edit_ui_color_);
  gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing, "we:edit-ui-v");
  gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing);
  gui::SetMargin(gui::Margin(edit_width_, 1, 0, 0));
  gui::EndGroup();
  gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing, "we:edit-ui-h");
  gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing);
  gui::SetMargin(gui::Margin(1, virtual_resolution_.y(), 0, 0));
  gui::EndGroup();

  gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing, "we:edit-ui-scroll");
  gui::SetMargin(gui::Margin(10, 10));
}

void EditorGui::FinishDrawEditView() {
  gui::EndGroup();  // we:edit-ui-v
  gui::EndGroup();  // we:edit-ui-h
  gui::EndGroup();  // we:edit-ui-scroll
  gui::EndScroll();
  gui::EndGroup();  // we:edit-ui-container
}

static const char* const kEditViewNames[] = {
    "Edit Entity", "List Entities", "Edit Proto", "List Protos", "Settings"};

void EditorGui::DrawTabs() {
  float kTabSpacing = 4;
  float kGrowSelectedTab = 4;
  float kTabButtonSize = 12;

  int new_edit_view = edit_view_;

  gui::StartGroup(gui::kLayoutOverlay, 0, "we:toolbar-bg");
  gui::PositionGroup(
      gui::kAlignLeft, gui::kAlignTop,
      vec2(virtual_resolution_.x() - edit_width_, config_->gui_toolbar_size()));
  CaptureMouseClicks();

  gui::StartGroup(gui::kLayoutHorizontalBottom, 0, "we:toolbar-fill");
  gui::SetMargin(gui::Margin(edit_width_, config_->gui_toolbar_size(), 0, 0));
  gui::ColorBackground(bg_edit_ui_color_);
  gui::EndGroup();  // we:toolbar-fill

  gui::StartGroup(gui::kLayoutHorizontalBottom, kTabSpacing, "we:toolbar");
  // do some math to get the tabs spaced evenly
  float num_tabs = kEditViewCount;
  float width_each = edit_width_ / num_tabs - kTabSpacing - 1;
  for (int i = 0; i < kEditViewCount; i++) {
    const char* view = kEditViewNames[i];
    // container for the tab
    gui::StartGroup(gui::kLayoutOverlay, 0,
                    (std::string("we:toolbar-tab-container-") + view).c_str());
    gui::StartGroup(gui::kLayoutHorizontalBottom, 0,
                    (std::string("we:toolbar-tab-overlay-") + view).c_str());
    float width_adjust = 0;
    float size_adjust = 0;
    if (edit_view_ == i) {
      gui::ColorBackground(mathfu::kZeros4f);
      gui::SetTextColor(text_button_color_);
      width_adjust = kTabSpacing;  // make selected tab slightly bigger
      size_adjust = kGrowSelectedTab;
    } else {
      gui::ColorBackground(bg_button_color_);
      gui::SetTextColor(text_normal_color_);
    }
    gui::SetMargin(gui::Margin(width_each + width_adjust,
                               config_->gui_toolbar_size(), 0, 0));
    auto event = gui::CheckEvent();
    if (event & gui::kEventWentUp) {
      new_edit_view = i;
    }
    gui::EndGroup();  // we:toolbar-tab-overlay-view
    gui::StartGroup(gui::kLayoutHorizontalBottom, 0,
                    (std::string("we:toolbar-tab-label-") + view).c_str());
    gui::Label(view, kTabButtonSize + size_adjust);
    gui::EndGroup();  // we:toolbar-tab-label-view
    gui::EndGroup();  // we:toolbar-tab-container-view
  }

  gui::EndGroup();  // we:toolbar
  gui::EndGroup();  // we:toolbar-bg
  if (new_edit_view >= 0 && new_edit_view < kEditViewCount) {
    edit_view_ = static_cast<EditView>(new_edit_view);
  }
}

void EditorGui::DrawSettingsUI() {
  const float kButtonSize = 30;
  if (TextButton(show_types_ ? "[Data types: On]" : "[Data types: Off]",
                 "we:types", kButtonSize) &
      gui::kEventWentUp)
    button_pressed_ = kToggleDataTypes;
  if (TextButton(show_physics_ ? "[Show physics: On]" : "[Show physics: Off]",
                 "we:physics", kButtonSize) &
      gui::kEventWentUp)
    button_pressed_ = kTogglePhysics;
  if (TextButton(expand_all_ ? "[Expand all: On]" : "[Expand all: Off]",
                 "we:expand", kButtonSize) &
      gui::kEventWentUp)
    button_pressed_ = kToggleExpandAll;
  if (edit_window_state_ == kNormal) {
    if (TextButton("[Maximize View]", "we:maximize", kButtonSize) &
        gui::kEventWentUp)
      button_pressed_ = kWindowMaximize;
  } else if (edit_window_state_ == kMaximized) {
    if (TextButton("[Restore View]", "we:restore", kButtonSize) &
        gui::kEventWentUp)
      button_pressed_ = kWindowRestore;
  }
  if (TextButton("[Hide View]", "we:hide", kButtonSize) & gui::kEventWentUp)
    button_pressed_ = kWindowHide;
}

void EditorGui::DrawEditEntityUI() {
  if (!edit_entity_) {
    gui::Label("No entity selected!", config_->gui_button_size());
  } else {
    changed_edit_entity_ = entity::EntityRef();

    for (entity::ComponentId id = 0; id <= entity_factory_->max_component_id();
         id++) {
      DrawEntityComponent(id);
    }
    DrawEntityFamily();

    if (changed_edit_entity_) {
      // Something during the course of rendering the UI caused the selected
      // entity to change, so let's select the new entity.
      SetEditEntity(changed_edit_entity_);
      changed_edit_entity_ = entity::EntityRef();
    }
  }
}

void EditorGui::DrawEntityListUI() {
  changed_edit_entity_ = entity::EntityRef();

  gui::StartGroup(gui::kLayoutHorizontalCenter, kSpacing,
                  "ws:entity-list-filter");
  gui::SetTextColor(text_normal_color_);
  gui::Label("Filter:", config_->gui_button_size());
  mathfu::vec2 size_vec =
      entity_list_filter_.length() > 0 ? vec2(0, 0) : vec2(kBlankEditWidth, 0);
  gui::SetTextColor(text_editable_color_);
  if (gui::Edit(config_->gui_button_size(), size_vec, "ws:entity-list-edit",
                &entity_list_filter_)) {
    keyboard_in_use_ = true;
  }
  gui::EndGroup();  // ws:entity-list-filter

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
  if (changed_edit_entity_ != entity::EntityRef()) {
    // select new entity
    if (changed_edit_entity_ == edit_entity_) {
      // select the same entity again, so change the tab mode
      edit_view_ = kEditEntity;
    }
    SetEditEntity(changed_edit_entity_);
    changed_edit_entity_ = entity::EntityRef();
  }
}

void EditorGui::DrawEntityComponent(entity::ComponentId id) {
  const int kTableNameSize = 30;
  const int kTableButtonSize = kTableNameSize - 8;

  // Check if we have a FlatbufferEditor for this component.
  entity::ComponentInterface* component = entity_manager_->GetComponent(id);
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

    gui::StartGroup(gui::kLayoutHorizontalBottom, kSpacing,
                    (table_name + "-container").c_str());
    gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                    (table_name + "-title").c_str());
    if (component_guis_[id]->flatbuffer_modified()) {
      gui::SetTextColor(text_modified_color_);
    } else {
      gui::SetTextColor(text_normal_color_);
    }
    if (has_data) {
      auto event = gui::CheckEvent();
      if (event & gui::kEventWentDown) {
        components_to_show_[id] = !components_to_show_[id];
      }
      if (event & gui::kEventHover) {
        gui::ColorBackground(bg_hover_color_);
      }
    } else {
      // gray out the table name since we can't click it
      gui::SetTextColor(text_disabled_color_);
    }
    gui::Label(table_name.c_str(), kTableNameSize);
    gui::SetTextColor(text_normal_color_);
    gui::EndGroup();  // $table_name-title
    MetaData* meta_data =
        entity_manager_->GetComponentData<MetaData>(edit_entity_);
    bool from_proto = meta_data
                          ? (meta_data->components_from_prototype.find(id) !=
                             meta_data->components_from_prototype.end())
                          : false;
    if (component_guis_[id]->flatbuffer_modified()) {
      if (TextButton("[Commit]", (table_name + "-commit-to-entity").c_str(),
                     kTableButtonSize) &
          gui::kEventWentUp) {
        auto_commit_component_ = id;
      }
      if (TextButton("[Revert]", (table_name + "-revert-entity").c_str(),
                     kTableButtonSize) &
          gui::kEventWentUp) {
        auto_revert_component_ = id;
      }
      if (TextButton("[Reload]", (table_name + "-reload-entity").c_str(),
                     kTableButtonSize) &
          gui::kEventWentUp) {
        auto_recreate_component_ = id;
      }
    } else if (has_data) {
      // Draw the title of the component table.
      gui::Label(from_proto ? "(from prototype)" : "(from entity)", 12);
    } else {
      // Component is present but wasn't exported by ExportRawData.
      // Usually that means it was automatically generated and doesn't
      // need to be edited by a human.
      gui::SetTextColor(text_disabled_color_);
      gui::Label("(not exported)", 12);
      gui::SetTextColor(text_normal_color_);
    }
    gui::EndGroup();  // $table_name-container

    if (has_data && (expand_all_ || components_to_show_[id])) {
      gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                      (table_name + "-contents").c_str());
      // Draw the actual edit controls for this component data.
      component_guis_[id]->set_show_types(show_types_);
      component_guis_[id]->set_expand_all(expand_all_);

      component_guis_[id]->Draw();
      gui::EndGroup();  // $table_name-contents
    }
  }
}

void EditorGui::DrawEntityFamily() {
  // Show a list of this entity's parents and children.
  TransformData* transform_data =
      entity_manager_->GetComponentData<TransformData>(edit_entity_);
  if (transform_data != nullptr) {
    gui::Label(" ", 20);  // insert some blank space
    if (transform_data->parent) {
      gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing, "we:parent");
      gui::Label("Parent:", 24);
      EntityButton(transform_data->parent, config_->gui_button_size());
      gui::EndGroup();  // we:parent
    }
    bool has_child = false;
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      entity::EntityRef& child = iter->owner;
      if (!has_child) {
        has_child = true;
        gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing, "we:children");
        gui::Label("Children:", 24);
      }
      EntityButton(child, config_->gui_button_size());
    }
    if (has_child) {
      gui::EndGroup();  // we:children
    }
  }
}

void EditorGui::EntityButton(const entity::EntityRef& entity, int size) {
  MetaData* meta_data = entity_manager_->GetComponentData<MetaData>(entity);
  std::string entity_id =
      meta_data
          ? entity_manager_->GetComponent<MetaComponent>()->GetEntityID(
                const_cast<entity::EntityRef&>(entity))
          : "Unknown entity ID";
  if (meta_data) {
    entity_id += "  (";
    entity_id += meta_data->prototype;
    entity_id += ")";
  }
  auto event =
      TextButton(entity_id.c_str(),
                 (std::string("we:entity-button-") + entity_id).c_str(), size);
  if (event & gui::kEventWentUp) {
    changed_edit_entity_ = entity;
  }
}

gui::Event EditorGui::TextButton(const char* text, const char* id, int size) {
  const float kMargin = 5;
  float text_size = size - 2 * kMargin;
  gui::StartGroup(gui::kLayoutHorizontalTop, size / 4, id);
  gui::SetMargin(gui::Margin(kMargin));
  auto event = gui::CheckEvent();
  if (event & ~gui::kEventHover) {
    mouse_in_window_ = true;
    gui::ColorBackground(bg_click_color_);
  } else if (event & gui::kEventHover) {
    gui::ColorBackground(bg_hover_color_);
  } else {
    gui::ColorBackground(bg_button_color_);
  }
  gui::SetTextColor(text_normal_color_);
  gui::Label(text, text_size);
  gui::EndGroup();  // $id
  return event;
}

}  // namespace editor
}  // namespace fpl
