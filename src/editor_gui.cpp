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
      resizable_flatbuffer_(nullptr),
      component_id_visiting_(0),
      force_propagate_(0),
      edit_width_(0),
      edit_window_state_(kNormal),
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
  ClearEntityModified();
  scroll_offset_ = mathfu::kZeros2i;

  bg_edit_ui_color_ = vec4(0.0f, 0.0f, 0.0f, 0.3f);
  bg_hover_color_ = vec4(0.5f, 0.5f, 0.5f, 0.8f);

  text_disabled_color_ = vec4(0.7f, 0.7f, 0.7f, 1.0f);
  text_normal_color_ = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  text_button_color_ = vec4(1.0f, 1.0f, 1.0f, 1.0f);

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
        edit_fields_.clear();
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
    edit_fields_.clear();
    scroll_offset_ = mathfu::kZeros2i;
    // set all components to unmodified
    ClearEntityModified();
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
  mouse_in_window_ = false;
  keyboard_in_use_ = false;
  GetVirtualResolution(&virtual_resolution_);

  if (edit_window_state_ == kMaximized)
    edit_width_ = virtual_resolution_.x();
  else if (edit_window_state_ == kNormal)
    edit_width_ = virtual_resolution_.x() / 3.0f;
  else if (edit_window_state_ == kHidden)
    edit_width_ = 0;

  gui::Run(*asset_manager_, *font_manager_, *input_system_, [&]() {
    PositionUI(1000, gui::kAlignLeft, gui::kAlignTop);
    gui::StartGroup(gui::kLayoutVerticalLeft, 0, "we:overall-ui");
    CaptureMouseClicks();
    gui::StartGroup(gui::kLayoutOverlayCenter, 0, "we:button-overlay");

    gui::StartGroup(gui::kLayoutHorizontalTop, 0, "we:button-bg");
    gui::ColorBackground(vec4(0.0f, 0.0f, 0.0f, 1.0f));
    gui::SetMargin(gui::Margin(virtual_resolution_.x(), kToolbarHeight, 0, 0));
    CaptureMouseClicks();
    gui::EndGroup();  // we:button-bg

    // Show a bunch of buttons along the top of the screen.
    gui::StartGroup(gui::kLayoutHorizontalTop, 10, "we:buttons");
    CaptureMouseClicks();
    if (edit_window_state_ == kNormal) {
      if (TextButton("[Maximize]", "we:maximize", 16) & gui::kEventWentDown)
        edit_window_state_ = kMaximized;
      if (TextButton("[Hide]", "we:hide", 16) & gui::kEventWentDown)
        edit_window_state_ = kHidden;
    } else {
      if (TextButton("[Restore]", "we:restore", 16) & gui::kEventWentDown)
        edit_window_state_ = kNormal;
    }
    if (TextButton(show_types_ ? "[Data types: On]" : "[Data types: Off]",
                   "we:types", 16) &
        gui::kEventWentDown)
      show_types_ = !show_types_;
    if (TextButton(expand_all_ ? "[Expand all: On]" : "[Expand all: Off]",
                   "we:expand", 16) &
        gui::kEventWentDown)
      expand_all_ = !expand_all_;
    if (TextButton(show_physics_ ? "[Show physics: On]" : "[Show physics: Off]",
                   "we:physics", 16) &
        gui::kEventWentDown)
      show_physics_ = !show_physics_;

    if (EntityModified()) {
      if (TextButton("[Commit Changes]", "we:commit", 16) &
          gui::kEventWentDown) {
        CommitEntityData();
      }
      if (TextButton("[Revert Changes]", "we:commit", 16) &
          gui::kEventWentDown) {
        edit_fields_.clear();
        ClearEntityModified();
      }

      if (force_propagate_ != 0) {
        entity::ComponentId id = force_propagate_;
        force_propagate_ = 0;
        PropagateEdits(id);
      }
    }
    gui::EndGroup();  // we:buttons
    gui::EndGroup();  // we:button-overlay

    if (edit_entity_ && edit_window_state_ != kHidden) {
      DrawEditEntityUI();
    }
    gui::EndGroup();  // we:overall-ui
  });
}

void EditorGui::CommitEntityData() {
  // Now go through the components that were modified, and for each one, add
  // it back as raw data.
  for (entity::ComponentId id = 0; id < component_buffer_modified_.size();
       id++) {
    if (entity_data_.find(id) != entity_data_.end()) {
      if (PropagateEdits(id)) {  // resets component_fields_modified_ and sets
                                 // component_buffers_modified_
        entity::ComponentInterface* component =
            entity_manager_->GetComponent(id);
        component->AddFromRawData(
            edit_entity_, flatbuffers::GetAnyRoot(entity_data_[id].data()));
        component_buffer_modified_[id] = false;
      }
    }
  }
  // All components have been saved out.
  event_manager_->BroadcastEvent(
      EditorEventPayload(EditorEventAction_EntityUpdated, edit_entity_));
}

bool EditorGui::PropagateEdits(entity::ComponentId id) {
  component_id_visiting_ = id;
  auto component = entity_manager_->GetComponent(id);
  if (component != nullptr &&
      component->GetComponentDataAsVoid(edit_entity_) != nullptr &&
      component_fields_modified_[id]) {
    std::vector<uint8_t>* raw_data =
        (entity_data_.find(id) != entity_data_.end()) ? &entity_data_[id]
                                                      : nullptr;
    if (raw_data != nullptr) {
      bool go_again;
      do {
        // EditFlatbuffer will set component_buffer_modified = true
        // if any data was modified.
        go_again = EditFlatbuffer(raw_data,
                                  entity_factory_->ComponentIdToTableName(id));
      } while (go_again);
      component_fields_modified_[id] = false;
    }
  }
  component_id_visiting_ = 0;
  return component_buffer_modified_[id];
}

void EditorGui::CaptureMouseClicks() {
  auto event = gui::CheckEvent();
  // Check for any event besides hover; if so, we'll take over mouse clicks.
  if (event & ~gui::kEventHover) {
    mouse_in_window_ = true;
  }
}

void EditorGui::DrawEditEntityUI() {
  changed_edit_entity_ = entity::EntityRef();

  gui::StartGroup(gui::kLayoutVerticalLeft, 0, "we:edit-ui");
  gui::StartScroll(vec2(edit_width_, virtual_resolution_.y() - kToolbarHeight),
                   &scroll_offset_);
  gui::ColorBackground(bg_edit_ui_color_);
  CaptureMouseClicks();

  gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing, "we:edit-ui-scroll");
  gui::ColorBackground(mathfu::kZeros4f);
  CaptureMouseClicks();
  gui::SetMargin(gui::Margin(10, 10));

  for (entity::ComponentId id = 0; id <= entity_factory_->max_component_id();
       id++) {
    DrawEntityComponent(id);
  }
  DrawEntityFamily();

  gui::EndGroup();  // we:edit-ui-scroll
  gui::EndScroll();
  gui::EndGroup();  // we:edit-ui

  if (changed_edit_entity_) {
    // Something during the course of rendering the UI caused the selected
    // entity to change, so let's select the new entity.
    SetEditEntity(changed_edit_entity_);
    changed_edit_entity_ = entity::EntityRef();
  }
}

void EditorGui::DrawEntityComponent(entity::ComponentId id) {
  entity::ComponentInterface* component = entity_manager_->GetComponent(id);
  if (component != nullptr &&
      component->GetComponentDataAsVoid(edit_entity_) != nullptr) {
    // If there is already component data loaded in entity_data_, use it.
    // Otherwise load it from the component and save it in entity_data_.
    if (entity_data_.find(id) == entity_data_.end()) {
      // not cached yet (or invalidated), let's load it
      std::vector<uint8_t> data;
      if (ReadDataFromEntity(edit_entity_, id, &data)) {
        entity_data_[id] = data;
      }
    }
    uint8_t* raw_data = (entity_data_.find(id) != entity_data_.end())
                            ? entity_data_[id].data()
                            : nullptr;

    std::string table_name = entity_factory_->ComponentIdToTableName(id);
    gui::StartGroup(gui::kLayoutHorizontalBottom, kSpacing,
                    (table_name + "-container").c_str());
    gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                    (table_name + "-title").c_str());
    gui::SetTextColor(text_normal_color_);
    if (raw_data != nullptr) {
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
    gui::Label(table_name.c_str(), 30);
    gui::SetTextColor(text_normal_color_);
    gui::EndGroup();  // $table_name-title
    MetaData* meta_data =
        entity_manager_->GetComponentData<MetaData>(edit_entity_);
    bool from_proto = meta_data
                          ? (meta_data->components_from_prototype.find(id) !=
                             meta_data->components_from_prototype.end())
                          : false;
    if (raw_data != nullptr) {
      // Draw the title of the component table.
      gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing,
                      (table_name + "-controls").c_str());

      /* TODO: "Apply changes" button,
       * "Save to prototype" button,
       * "Revert to prototype" button.
       */
      gui::Label(from_proto ? "(from prototype)" : "(from entity)", 12);
      gui::EndGroup();  // $table_name-controls
    } else {
      // Component is present but wasn't exported by ExportRawData.
      // Usually that means it was automatically generated and doesn't
      // need to be edited by a human.
      gui::SetTextColor(text_disabled_color_);
      gui::Label("(not exported)", 12);
      gui::SetTextColor(text_normal_color_);
    }
    gui::EndGroup();  // $table_name-container

    // If we have actual data to show, let's show it!
    if (raw_data != nullptr) {
      if (expand_all_ || components_to_show_[id]) {
        gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                        (table_name + "-contents").c_str());
        DrawFlatbuffer(raw_data, entity_factory_->ComponentIdToTableName(id));
        gui::EndGroup();  // $table_name-contents
        gui::SetTextColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (raw_data != nullptr) {
          auto event = gui::CheckEvent();
          if (event & gui::kEventWentDown) {
            components_to_show_[id] = !components_to_show_[id];
          }
          if (event & gui::kEventHover) {
            gui::ColorBackground(vec4(0.5f, 0.5f, 0.5f, 0.8f));
          }
        } else {
          // gray out the table name since we can't click it
          gui::SetTextColor(vec4(0.7f, 0.7f, 0.7f, 1.0f));
        }
        gui::Label(table_name.c_str(), 30);
        gui::SetTextColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
        gui::EndGroup();  // $table_name-title
        MetaData* meta_data =
            entity_manager_->GetComponentData<MetaData>(edit_entity_);
        bool from_proto =
            meta_data ? (meta_data->components_from_prototype.find(id) !=
                         meta_data->components_from_prototype.end())
                      : false;
        if (raw_data != nullptr) {
          gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing,
                          (table_name + "-controls").c_str());

          /* TODO: "Apply changes" button,
           * "Save to prototype" button,
           * "Revert to prototype" button.
           */
          gui::Label(from_proto ? "(from prototype)" : "(from entity)", 12);
          gui::EndGroup();  // $table_name-controls
        } else {
          // Component is present but wasn't exported by ExportRawData.
          // Usually that means it was automatically generated and doesn't
          // need to be edited by a human.
          gui::SetTextColor(vec4(0.7f, 0.7f, 0.7f, 1.0f));
          gui::Label("(not exported)", 12);
          gui::SetTextColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        gui::EndGroup();  // $table_name-container
        if (raw_data != nullptr) {
          if (expand_all_ || components_to_show_[id]) {
            gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                            (table_name + "-contents").c_str());
            component_id_visiting_ = id;
            DrawFlatbuffer(raw_data,
                           entity_factory_->ComponentIdToTableName(id));
            component_id_visiting_ = 0;
            gui::EndGroup();  // $table_name-contents
          }
        }
      }
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
      EntityButton(transform_data->parent, config_->gui_children_ui_size());
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
      EntityButton(child, config_->gui_children_ui_size());
    }
    if (has_child) {
      gui::EndGroup();  // we:children
    }
  }
}

bool EditorGui::EntityButton(entity::EntityRef& entity, int size) {
  bool ret = false;
  entity::EntityRef new_entity;
  MetaData* meta_data = entity_manager_->GetComponentData<MetaData>(entity);
  if (meta_data) {
    std::string entity_id =
        entity_manager_->GetComponent<MetaComponent>()->GetEntityID(entity);
    gui::StartGroup(gui::kLayoutHorizontalTop, 0,
                    (std::string("we:entity-button-") + entity_id).c_str());
    auto event = gui::CheckEvent();
    if (event & ~gui::kEventHover) {
      gui::ColorBackground(vec4(0.5f, 0.5f, 0.5f, 0.8f));
    }
    if (event & gui::kEventWentDown) {
      changed_edit_entity_ = entity;
      ret = true;
    }
    gui::Label(
        (entity_id + "  (" +
         entity_manager_->GetComponentData<MetaData>(entity)->prototype + ")")
            .c_str(),
        size);
    gui::EndGroup();  // we:entity-button-$entity_id
  }
  return ret;
}

bool EditorGui::ReadDataFromEntity(entity::EntityRef& entity,
                                   entity::ComponentId component_id,
                                   std::vector<uint8_t>* output_vector) const {
  auto services = entity_manager_->GetComponent<CommonServicesComponent>();
  auto component = entity_manager_->GetComponent(component_id);
  if (component != nullptr &&
      component->GetComponentDataAsVoid(entity) != nullptr) {
    bool prev_force_defaults = services->export_force_defaults();
    // Force all default fields to have values that the user can edit
    services->set_export_force_defaults(true);
    // Export the entity's raw data.
    auto raw_data = component->ExportRawData(entity);
    services->set_export_force_defaults(prev_force_defaults);

    if (raw_data == nullptr) return false;
    auto schema = reflection::GetSchema(schema_data_->c_str());
    if (schema == nullptr) return false;
    std::string table_name =
        entity_factory_->ComponentIdToTableName(component_id);
    if (table_name == "") return false;
    auto table_def = schema->objects()->LookupByKey(table_name.c_str());
    if (table_def == nullptr) return false;

    if (output_vector != nullptr) {
      flatbuffers::FlatBufferBuilder fbb;
      auto table = flatbuffers::CopyTable(
          fbb, *schema, *table_def, *flatbuffers::GetAnyRoot(raw_data.get()));
      fbb.Finish(table);
      *output_vector = {fbb.GetBufferPointer(),
                        fbb.GetBufferPointer() + fbb.GetSize()};
    }
    return true;
  }
  return false;
}

bool EditorGui::DrawFlatbuffer(uint8_t* data, const char* table_name) {
  if (data == nullptr) return false;
  auto schema = reflection::GetSchema(schema_data_->c_str());
  if (schema == nullptr) return false;
  auto table_def = schema->objects()->LookupByKey(table_name);
  if (table_def == nullptr) return false;
  VisitFlatbufferTable(kDraw, *schema, *table_def,
                       *flatbuffers::GetAnyRoot(data), table_name);
  return true;
}

bool EditorGui::EditFlatbuffer(std::vector<uint8_t>* data,
                               const char* table_name) {
  if (data == nullptr) return false;
  auto schema = reflection::GetSchema(schema_data_->c_str());
  if (schema == nullptr) return false;
  auto table_def = schema->objects()->LookupByKey(table_name);
  if (table_def == nullptr) return false;
  resizable_flatbuffer_ = data;
  bool b =
      VisitFlatbufferTable(kEdit, *schema, *table_def,
                           *flatbuffers::GetAnyRoot(data->data()), table_name);
  resizable_flatbuffer_ = nullptr;
  return b;
}

static const char kStructSep[] = ", ";
static const char kStructBegin[] = "< ";
static const char kStructEnd[] = " >";

int64_t EditorGui::ReadInt64(const std::string& str,
                             size_t* num_characters_output) {
  int64_t output;
  char* ptr_output;
#ifdef _MSC_VER
  output = _strtoui64(str.c_str(), &ptr_output, 0);
#else
  output = strtoull(str.c_str(), &ptr_output, 0);
#endif
  if (num_characters_output != nullptr)
    *num_characters_output = ptr_output - str.c_str();
  return output;
}

double EditorGui::ReadDouble(const std::string& str,
                             size_t* num_characters_output) {
  double output;
  char* ptr_output;
  output = strtod(str.c_str(), &ptr_output);
  if (num_characters_output != nullptr)
    *num_characters_output = ptr_output - str.c_str();
  return output;
}

std::string EditorGui::ExtractStructDef(const std::string& str) {
  int nest_level = 0;
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] == '<')
      nest_level++;
    else if (str[i] == '>') {
      nest_level--;
      if (nest_level == 0) {
        std::string ret = str.substr(1, i - 1);
        return ret.length() == 0 ? " " : ret;
      }
    }
    if (nest_level < 0) return "";
  }
  return "";
}

std::string EditorGui::ParseScalar(const std::string& str,
                                   reflection::BaseType base_type,
                                   void* output_buffer) {
  size_t characters_used = 0;
  switch (base_type) {
    case reflection::Bool: {
      bool output = ReadInteger<bool>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<bool*>(output_buffer) = output;
      break;
    }
    case reflection::UByte: {
      unsigned char output = ReadInteger<unsigned char>(str, &characters_used);
      if (output_buffer)
        *reinterpret_cast<unsigned char*>(output_buffer) = output;
      break;
    }
    case reflection::Byte: {
      char output = ReadInteger<char>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<char*>(output_buffer) = output;
      break;
    }
    case reflection::Short: {
      short output = ReadInteger<short>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<short*>(output_buffer) = output;
      break;
    }
    case reflection::UShort: {
      unsigned short output =
          ReadInteger<unsigned short>(str, &characters_used);
      if (output_buffer)
        *reinterpret_cast<unsigned short*>(output_buffer) = output;
      break;
    }
    case reflection::Int: {
      int output = ReadInteger<int>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<int*>(output_buffer) = output;
      break;
    }
    case reflection::UInt: {
      unsigned int output = ReadInteger<unsigned int>(str, &characters_used);
      if (output_buffer)
        *reinterpret_cast<unsigned int*>(output_buffer) = output;
      break;
    }
    case reflection::Long: {
      long output = ReadInteger<long>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<long*>(output_buffer) = output;
      break;
    }
    case reflection::ULong: {
      unsigned long output = ReadInteger<unsigned long>(str, &characters_used);
      if (output_buffer)
        *reinterpret_cast<unsigned long*>(output_buffer) = output;
      break;
    }
    case reflection::Float: {
      float output = ReadFloatingPoint<float>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<float*>(output_buffer) = output;
      break;
    }
    case reflection::Double: {
      double output = ReadFloatingPoint<double>(str, &characters_used);
      if (output_buffer) *reinterpret_cast<double*>(output_buffer) = output;
      break;
    }
    default: {
      return str;
      break;
    }
  }
  return str.substr(characters_used);
}

static std::string StripCommasAndWhitespaceAtStart(const std::string& str) {
  // Strip away commas and whitespace from the start
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != ',' && str[i] != ' ') {
      return str.substr(i);
    }
  }
  return "";  // everything was consumed!
}

bool EditorGui::ParseStringIntoStruct(const std::string& struct_def,
                                      const reflection::Schema& schema,
                                      const reflection::Object& objectdef,
                                      void* struct_ptr) {
  std::string str = ExtractStructDef(struct_def);
  if (str.length() == 0) {
    LogInfo("Struct parse error: overall struct def");
    return false;
  }

  for (auto sit = objectdef.fields()->begin(); sit != objectdef.fields()->end();
       ++sit) {
    const reflection::Field& fielddef = **sit;
    void* ptr = struct_ptr
                    ? static_cast<uint8_t*>(struct_ptr) + fielddef.offset()
                    : nullptr;
    switch (fielddef.type()->base_type()) {
      default: {
        // scalar
        std::string new_str =
            ParseScalar(str, fielddef.type()->base_type(), ptr);
        if (new_str == str) {
          LogInfo("Struct parse error: scalar");
          return false;
        }
        str = new_str;
        break;
      }
      case reflection::Obj: {
        // sub-struct
        auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());
        if (subobjdef.is_struct()) {
          std::string substr = ExtractStructDef(str);
          if (substr.length() == 0) {
            LogInfo("Struct parse error: extracting substruct.");
            return false;
          }
          if (!ParseStringIntoStruct(substr, schema, subobjdef, ptr)) {
            LogInfo("Struct parse error: substruct");
            return false;
          }
          str += substr.length();
        }
      }
    }
    str = StripCommasAndWhitespaceAtStart(str);
    // are we out of string?
    if (str.length() == 0) break;
  }
  return true;
}

std::string EditorGui::GetScalarAsString(reflection::BaseType base_type,
                                         const void* ptr) {
  switch (base_type) {
    case reflection::Bool: {
      return NumToString(*reinterpret_cast<const bool*>(ptr));
      break;
    }
    case reflection::UByte: {
      return NumToString(*reinterpret_cast<const unsigned char*>(ptr));
      break;
    }
    case reflection::Byte: {
      return NumToString(*reinterpret_cast<const char*>(ptr));
      break;
    }
    case reflection::Short: {
      return NumToString(*reinterpret_cast<const short*>(ptr));
      break;
    }
    case reflection::UShort: {
      return NumToString(*reinterpret_cast<const unsigned short*>(ptr));
      break;
    }
    case reflection::Int: {
      return NumToString(*reinterpret_cast<const int*>(ptr));
      break;
    }
    case reflection::UInt: {
      return NumToString(*reinterpret_cast<const unsigned int*>(ptr));
      break;
    }
    case reflection::Long: {
      return NumToString(*reinterpret_cast<const long*>(ptr));
      break;
    }
    case reflection::ULong: {
      return NumToString(*reinterpret_cast<const unsigned long*>(ptr));
      break;
    }
    case reflection::Float: {
      return NumToString(*reinterpret_cast<const float*>(ptr));
      break;
    }
    case reflection::Double: {
      return NumToString(*reinterpret_cast<const double*>(ptr));
      break;
    }
    default: {
      return "(unknown type)";
      break;
    }
  }
}

std::string EditorGui::StructToString(const reflection::Schema& schema,
                                      const reflection::Object& objectdef,
                                      const void* struct_ptr,
                                      bool field_names_only) {
  if (struct_ptr == nullptr) return "(null)";

  std::string output = kStructBegin;
  for (auto sit = objectdef.fields()->begin(); sit != objectdef.fields()->end();
       ++sit) {
    if (sit != objectdef.fields()->begin()) {
      output += kStructSep;
    }
    const reflection::Field& fielddef = **sit;
    const void* ptr =
        static_cast<const uint8_t*>(struct_ptr) + fielddef.offset();
    switch (fielddef.type()->base_type()) {
      case reflection::Obj: {
        auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());
        // It must be a struct, because structs can only contain scalars and
        // other structs, but let's just make sure.
        if (subobjdef.is_struct()) {
          if (field_names_only) {
            output += fielddef.name()->str();
            output += ":";
          }
          output += StructToString(schema, subobjdef, ptr, field_names_only);
        }
        break;
      }
      default: {
        // scalar
        if (field_names_only) {
          output += fielddef.name()->str();
        } else {
          output += GetScalarAsString(fielddef.type()->base_type(), ptr);
        }
        break;
      }
    }
  }
  output += kStructEnd;
  return output;
}

bool EditorGui::VisitField(VisitMode mode, const std::string& name,
                           const std::string& value, const std::string& type,
                           const std::string& comment, const std::string& id) {
  if (mode != kDrawReadOnly) {
    if (edit_fields_.find(id) == edit_fields_.end()) {
      edit_fields_[id] = value;
    }
  }
  std::string edit_id = id + "-edit";
  if (mode == kDraw || mode == kDrawReadOnly) {
    gui::StartGroup(gui::kLayoutHorizontalCenter, kSpacing,
                    (id + "-container").c_str());
    std::string name_full = name;
    if (type != "" && show_types_) {
      name_full += "<";
      name_full += type;
      name_full += ">";
    }
    name_full += ": ";
    gui::Label(name_full.c_str(), config_->gui_edit_ui_size());
  }
  if (mode == kDrawReadOnly) {
    // gray, so it appears as read-only
    gui::SetTextColor(vec4(0.8f, 0.8f, 0.8f, 1.0f));
  } else {
    if (edit_fields_[id] != value) {
      // yellow
      if (mode == kDraw) gui::SetTextColor(vec4(1.0f, 1.0f, 0.0f, 1.0f));
      component_fields_modified_[component_id_visiting_] = true;
      if (mode == kEdit) {
        LogInfo("VisitField: Setting '%s' to '%s' (was: '%s')", id.c_str(),
                edit_fields_[id].c_str(), value.c_str());
        return true;  // Return true if the field was changed.
      }
    } else {
      // green
      if (mode == kDraw) gui::SetTextColor(vec4(0.0f, 1.0f, 0.0f, 1.0f));
    }
  }
  if (mode == kDraw) {
    vec2 edit_vec = vec2(0, 0);
    if (edit_fields_[id].length() == 0) {
      edit_vec.x() = kBlankStringEditWidth;
    }
    if (gui::Edit(config_->gui_edit_ui_size(), vec2(0, 0), edit_id.c_str(),
                  &edit_fields_[id])) {
      keyboard_in_use_ = true;
    }
  } else if (mode == kDrawReadOnly) {
    gui::Label(value.c_str(), config_->gui_edit_ui_size());
  }
  if (mode == kDraw || mode == kDrawReadOnly) {
    gui::SetTextColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (comment != "") {
      gui::Label(comment.c_str(), config_->gui_edit_ui_size());
    }
    gui::EndGroup();  // $id-container
  }
  return false;  // nothing changed
}

bool EditorGui::VisitSubtable(VisitMode mode, const std::string& field,
                              const std::string& type,
                              const std::string& comment, const std::string& id,
                              const reflection::Schema& schema,
                              const reflection::Object& subobjdef,
                              flatbuffers::Table& subtable) {
  if (mode == kEdit || expand_all_ ||
      expanded_subtables_.find(id) != expanded_subtables_.end()) {
    // subtable, not a struct
    if (mode == kDraw || mode == kDrawReadOnly) {
      gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing,
                      (id + "-field").c_str());
      gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                      (id + "-fieldName").c_str());
      auto event = gui::CheckEvent();
      std::string name_full = field;
      if (type != "" && show_types_) {
        name_full += "<";
        name_full += type;
        name_full += ">";
      }
      name_full += ": ";
      gui::Label(name_full.c_str(), config_->gui_edit_ui_size());
      if (event & gui::kEventWentDown) {
        if (!expand_all_) expanded_subtables_.erase(id);
      }
      gui::EndGroup();  // $id-fieldName
      gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                      (id + "-nestedTable").c_str());
    }
    if (VisitFlatbufferTable(mode, schema, subobjdef, subtable, id)) {
      return true;
    }
    if (mode == kDraw || mode == kDrawReadOnly) {
      gui::EndGroup();  // $id-nestedTable
      if (comment != "") {
        gui::Label((std::string("(") + comment + ")").c_str(),
                   config_->gui_edit_ui_size());
      }
      gui::EndGroup();  // $id-field
    }
  } else {
    gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing,
                    (id + "-field").c_str());
    auto event = gui::CheckEvent();
    if (event & gui::kEventWentDown) {
      expanded_subtables_.insert(id);
    }
    gui::StartGroup(gui::kLayoutHorizontalTop, kSpacing,
                    (id + "-fieldName").c_str());
    std::string name_full = field;
    if (type != "" && show_types_) {
      name_full += "<";
      name_full += type;
      name_full += ">";
    }
    name_full += ": ";
    gui::Label(name_full.c_str(), config_->gui_edit_ui_size());
    gui::EndGroup();  // $id-fieldName
    gui::StartGroup(gui::kLayoutVerticalLeft, kSpacing,
                    (id + "-nestedTable").c_str());
    gui::Label("...", config_->gui_edit_ui_size());
    if (comment != "") {
      gui::Label((std::string("(") + comment + ")").c_str(),
                 config_->gui_edit_ui_size());
    }
    gui::EndGroup();  // $id-nestedTable
    gui::EndGroup();  // $id-field
  }
  return false;  // nothing changed
}

bool EditorGui::VisitFlatbufferTable(VisitMode mode,
                                     const reflection::Schema& schema,
                                     const reflection::Object& objectdef,
                                     flatbuffers::Table& table,
                                     const std::string& id) {
  auto fielddefs = objectdef.fields();
  for (auto it = fielddefs->begin(); it != fielddefs->end(); ++it) {
    const reflection::Field& fielddef = **it;
    if (!table.CheckField(fielddef.offset())) continue;
    if (VisitFlatbufferField(mode, schema, fielddef, objectdef, table, id))
      return true;
  }
  return false;  // no resize occurred
}

bool EditorGui::VisitFlatbufferField(VisitMode mode,
                                     const reflection::Schema& schema,
                                     const reflection::Field& fielddef,
                                     const reflection::Object& objectdef,
                                     flatbuffers::Table& table,
                                     const std::string& id) {
  std::string new_id = id + "." + fielddef.name()->str();
  auto base_type = fielddef.type()->base_type();
  switch (base_type) {
    default: {
      VisitFlatbufferScalar(mode, schema, fielddef, table, new_id);
      // Mutated scalars never resize, so we can ignore the "true" return value.
      break;
    }
    case reflection::String: {
      if (VisitFlatbufferString(mode, schema, fielddef, table, new_id))
        return true;  // Mutated strings may need to be resized
      break;
    }
    case reflection::Obj: {
      auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());

      if (subobjdef.is_struct()) {
        VisitFlatbufferStruct(mode, schema, fielddef, subobjdef, table, new_id);
        // Mutated structs never need to be resized
      } else {
        if (fielddef.offset() != 0) {
          flatbuffers::Table& subtable =
              const_cast<flatbuffers::Table&>(*GetFieldT(table, fielddef));
          if (VisitSubtable(mode, fielddef.name()->str(),
                            subobjdef.name()->str(), "", new_id, schema,
                            subobjdef, subtable))
            return true;  // Mutated tables may need to be resized
        }
      }
      break;
    }
    case reflection::Union: {
      if (VisitFlatbufferUnion(mode, schema, fielddef, objectdef, table,
                               new_id))
        return true;  // Mutated unions may need to be resized
      break;
    }
    case reflection::Vector: {
      if (VisitFlatbufferVector(mode, schema, fielddef, table, new_id))
        return true;
      break;
    }
  }
  return false;  // no resize occurred
}

// There are way faster ways to do popcount, but this one is simple.
static unsigned int NumBitsSet(uint64_t n) {
  unsigned int total = 0;
  for (uint64_t i = 0; i < 8 * sizeof(uint64_t); i++) {
    if (n & (1L << i)) total++;
  }
  return total;
}

std::string EditorGui::GetEnumTypeAndValue(const reflection::Schema& schema,
                                           const reflection::Field& fielddef,
                                           const std::string& value,
                                           std::string* type,
                                           std::string* value_name) {
  *type = "";
  *value_name = "";
  std::string scalar_value = value;
  if (fielddef.type()->index() > -1) {
    // we're an enum
    auto e = schema.enums()->Get(fielddef.type()->index());
    auto num = flatbuffers::StringToInt(scalar_value.c_str());
    *type = e->name()->str();

    bool found = false;
    bool is_bit_flags = true;
    for (uoffset_t ev = 0; ev < e->values()->size(); ev++) {
      if (is_bit_flags && NumBitsSet(e->values()->Get(ev)->value()) > 1)
        is_bit_flags = false;
      if (e->values()->Get(ev)->value() == num) {
        *value_name = e->values()->Get(ev)->name()->str();
        found = true;
        break;
      }
    }
    if (!found) {
      if (is_bit_flags) {
        uint64_t bit_num = num;
        if (bit_num == 0) {
          // no value for 0 - show that the bit flags are blank
          *value_name += "-blank-";
        } else {
          for (uoffset_t ev = 0; ev < e->values()->size(); ev++) {
            uint64_t bit = e->values()->Get(ev)->value();
            if (bit & bit_num) {
              if (value_name->length() > 0) *value_name += " | ";
              *value_name += e->values()->Get(ev)->name()->str();
              bit_num &= ~bit;
            }
          }
          if (bit_num != 0 && value_name->length() > 0) {
            // bits were left over, so that doesn't work for us
            *value_name += " | ???";
          }
        }
      } else {
        // couldn't find it as a bit flag value
        *value_name = "???";
      }
    }
    scalar_value = flatbuffers::NumToString(num);
  }
  if (value_name->length() > 0) {
    *value_name = std::string("(") + *value_name + ")";
  }
  return scalar_value;
}

bool EditorGui::VisitFlatbufferScalar(VisitMode mode,
                                      const reflection::Schema& schema,
                                      const reflection::Field& fielddef,
                                      flatbuffers::Table& table,
                                      const std::string& id) {
  // Check for enum...
  std::string value, enum_value, type, comment;
  enum_value = value = GetAnyFieldS(table, fielddef, schema);
  if (mode != kDrawReadOnly && edit_fields_.find(id) != edit_fields_.end()) {
    // if we've typed in a new value, show the enum value for what we've typed
    enum_value = edit_fields_[id].c_str();
  }
  GetEnumTypeAndValue(schema, fielddef, enum_value, &type, &comment);

  if (VisitField(mode, fielddef.name()->str(), value, type, comment, id)) {
    // Read the value of edit_fields_[id] and put it into the current table.
    SetAnyFieldS(&table, fielddef, edit_fields_[id].c_str());
    component_buffer_modified_[component_id_visiting_] = true;
  }
  return false;  // mutated scalars will never need to resize
}

bool EditorGui::VisitFlatbufferString(VisitMode mode,
                                      const reflection::Schema& schema,
                                      const reflection::Field& fielddef,
                                      flatbuffers::Table& table,
                                      const std::string& id) {
  std::string str_text, comment;
  if (fielddef.offset() == 0)
    comment = "(no value)";
  else
    str_text = GetFieldS(table, fielddef)->str();

  if (VisitField(mode, fielddef.name()->str(), str_text, "string", comment,
                 id)) {
    // Read the value of edit_fields_[id] and put it into the current table.
    flatbuffers::String* str =
        table.GetPointer<flatbuffers::String*>(fielddef.offset());
    SetString(
        schema, edit_fields_[id], str, resizable_flatbuffer_,
        schema.objects()->LookupByKey(
            entity_factory_->ComponentIdToTableName(component_id_visiting_)));
    component_buffer_modified_[component_id_visiting_] = true;
    return true;
  }
  return false;
}

bool EditorGui::VisitFlatbufferStruct(VisitMode mode,
                                      const reflection::Schema& schema,
                                      const reflection::Field& fielddef,
                                      const reflection::Object& objectdef,
                                      flatbuffers::Table& table,
                                      const std::string& id) {
  uint8_t* struct_ptr = table.GetStruct<uint8_t*>(fielddef.offset());
  if (VisitField(mode, fielddef.name()->str(),
                 StructToString(schema, objectdef, struct_ptr, false),
                 objectdef.name()->str(),
                 show_types_
                     ? StructToString(schema, objectdef, struct_ptr, true)
                     : "",
                 id)) {
    // check if the field is valid first
    bool valid =
        ParseStringIntoStruct(edit_fields_[id], schema, objectdef, nullptr);
    if (valid) {
      LogInfo("Struct '%s' WAS valid for %s.", edit_fields_[id].c_str(),
              id.c_str());
      ParseStringIntoStruct(edit_fields_[id], schema, objectdef, struct_ptr);
      component_buffer_modified_[component_id_visiting_] = true;
    } else {
      LogInfo("Struct '%s' was not valid for %s.", edit_fields_[id].c_str(),
              id.c_str());
      // TODO: mark invalid fields in red?
    }
  }
  return false;
}

bool EditorGui::VisitFlatbufferUnion(VisitMode mode,
                                     const reflection::Schema& schema,
                                     const reflection::Field& fielddef,
                                     const reflection::Object& objectdef,
                                     flatbuffers::Table& table,
                                     const std::string& id) {
  auto& subobjdef = GetUnionType(schema, objectdef, fielddef, table);
  if (fielddef.offset() > 0) {
    flatbuffers::Table& subtable =
        const_cast<flatbuffers::Table&>(*GetFieldT(table, fielddef));
    return VisitSubtable(mode, fielddef.name()->str(), subobjdef.name()->str(),
                         "", id, schema, subobjdef, subtable);
  } else {
    return false;
  }
}

// Alternative Flatbuffers ResizeVector that just sets the new bytes to 0.
// TODO: Consider putting this in Flatbuffers' reflection.h?
inline void ResizeVector(const reflection::Schema& schema, uoffset_t newsize,
                         const reflection::BaseType elem_type,
                         const Vector<Offset<void>>* vec,
                         std::vector<uint8_t>* flatbuf,
                         const reflection::Object* root_table = nullptr) {
  size_t type_size = flatbuffers::GetTypeSize(elem_type);
  auto delta_elem = static_cast<int>(newsize) - static_cast<int>(vec->size());
  auto delta_bytes = delta_elem * type_size;
  auto vec_start = reinterpret_cast<const uint8_t*>(vec) - flatbuf->data();
  auto start = static_cast<uoffset_t>(vec_start + sizeof(uoffset_t) +
                                      type_size * vec->size());
  if (delta_bytes) {
    flatbuffers::ResizeContext(schema, start, delta_bytes, flatbuf, root_table);
    flatbuffers::WriteScalar(flatbuf->data() + vec_start,
                             newsize);  // Length field.
    // Set new elements to "val".
    for (int i = 0; i < delta_elem; i++) {
      auto loc = flatbuf->data() + start + i * type_size;
      memset(loc, 0, type_size);
    }
  }
}

bool EditorGui::VisitFlatbufferVector(VisitMode mode,
                                      const reflection::Schema& schema,
                                      const reflection::Field& fielddef,
                                      flatbuffers::Table& table,
                                      const std::string& id) {
  if (fielddef.offset() == 0) return false;
  auto vec = table.GetPointer<Vector<Offset<Table>>*>(fielddef.offset());
  auto element_base_type = fielddef.type()->element();
  auto elemobjectdef = element_base_type == reflection::Obj
                           ? schema.objects()->Get(fielddef.type()->index())
                           : nullptr;

  std::string idx = ".size";
  if (mode == kDraw || mode == kDrawReadOnly)
    gui::StartGroup(gui::kLayoutHorizontalCenter, 8,
                    (id + idx + "-commit").c_str());
  if (VisitField(mode, fielddef.name()->str() + idx,
                 flatbuffers::NumToString(vec->size()), "size_t", "",
                 id + idx)) {
    uoffset_t new_size =
        flatbuffers::StringToInt(edit_fields_[id + idx].c_str());
    auto vec_v = reinterpret_cast<const Vector<Offset<void>>*>(vec);
    ResizeVector(
        schema, new_size, element_base_type, vec_v, resizable_flatbuffer_,
        schema.objects()->LookupByKey(
            entity_factory_->ComponentIdToTableName(component_id_visiting_)));
    if (mode == kDraw || mode == kDrawReadOnly)
      gui::EndGroup();  // $id$idx-commit
    return true;
  }
  if (flatbuffers::StringToInt(edit_fields_[id + idx].c_str()) != vec->size()) {
    if (TextButton("click to resize", (id + idx + "resize").c_str(),
                   config_->gui_edit_ui_size()) &
        gui::kEventWentDown) {
      force_propagate_ = component_id_visiting_;
      component_buffer_modified_[component_id_visiting_] = true;
    }
  }
  if (mode == kDraw || mode == kDrawReadOnly)
    gui::EndGroup();  // $id$idx-commit
  switch (element_base_type) {
    case reflection::String: {
      // vector of strings
      auto vec_s = reinterpret_cast<Vector<Offset<flatbuffers::String>>*>(vec);
      uoffset_t i;
      for (i = 0; i < vec_s->size(); i++) {
        std::string idx = "[" + flatbuffers::NumToString(i) + "]";
        const flatbuffers::String* str = vec_s->Get(i);
        if (VisitField(mode, fielddef.name()->str() + idx, str->str(), "string",
                       "", id + idx)) {
          SetString(schema, edit_fields_[id + idx], str, resizable_flatbuffer_,
                    schema.objects()->LookupByKey(
                        entity_factory_->ComponentIdToTableName(
                            component_id_visiting_)));
          component_buffer_modified_[component_id_visiting_] = true;
          return true;
        }
      }
      break;
    }
    case reflection::Obj: {
      if (!elemobjectdef->is_struct()) {
        // vector of tables
        for (uoffset_t i = 0; i < vec->size(); i++) {
          std::string idx = "[" + flatbuffers::NumToString(i) + "]";
          flatbuffers::Table& tableelem =
              const_cast<flatbuffers::Table&>(*vec->Get(i));
          if (VisitSubtable(mode, fielddef.name()->str() + idx,
                            elemobjectdef->name()->str(), "", id + idx, schema,
                            *elemobjectdef, tableelem))
            return true;  // Mutated tables may require resize.
        }
      } else {
        // vector of structs
        auto element_size = elemobjectdef->bytesize();
        uint8_t* start = static_cast<uint8_t*>(vec->Data());
        int i = 0;
        for (uint8_t* ptr = start; ptr < start + (element_size * vec->size());
             ptr += element_size) {
          std::string idx = "[" + flatbuffers::NumToString(i) + "]";
          if (VisitField(mode, fielddef.name()->str() + idx,
                         StructToString(schema, *elemobjectdef, ptr, false),
                         elemobjectdef->name()->str(),
                         show_types_
                             ? StructToString(schema, *elemobjectdef, ptr, true)
                             : "",
                         id + idx)) {
            // check if the field is valid first
            bool valid = ParseStringIntoStruct(edit_fields_[id + idx], schema,
                                               *elemobjectdef, nullptr);
            if (valid) {
              LogInfo("Struct '%s' WAS valid for %s.",
                      edit_fields_[id + idx].c_str(), (id + idx).c_str());
              ParseStringIntoStruct(edit_fields_[id + idx], schema,
                                    *elemobjectdef, ptr);
              component_buffer_modified_[component_id_visiting_] = true;
            } else {
              LogInfo("Struct '%s' was not valid for %s.",
                      edit_fields_[(id + idx)].c_str(), id.c_str());
              // TODO: Mark invalid fields in red?
            }
          }
          i++;
        }
      }
      break;
    }
    default: {
      // vector of scalars
      std::string output;
      auto element_size = flatbuffers::GetTypeSize(element_base_type);
      uint8_t* start = static_cast<uint8_t*>(vec->Data());
      int i = 0;
      for (uint8_t* ptr = start; ptr < start + (element_size * vec->size());
           ptr += element_size) {
        std::string idx = "[" + flatbuffers::NumToString(i) + "]";
        std::string enum_type, enum_hint;
        std::string value = GetScalarAsString(element_base_type, ptr);
        if (edit_fields_.find(id + idx) != edit_fields_.end()) {
          value = edit_fields_[id + idx];
        }
        value = GetEnumTypeAndValue(schema, fielddef, value, &enum_type,
                                    &enum_hint);

        if (VisitField(mode, fielddef.name()->str() + idx, value, enum_type,
                       enum_hint, id + idx)) {
          // Handle the same way as VisitFlatbufferScalar does.
          ParseScalar(edit_fields_[id + idx], element_base_type, ptr);
          component_buffer_modified_[component_id_visiting_] = true;
        }
        i++;
      }
      break;
    }
  }
  return false;
}

gui::Event EditorGui::TextButton(const char* text, const char* id, int size) {
  gui::StartGroup(gui::kLayoutOverlayCenter, size / 4, id);
  gui::SetMargin(gui::Margin(5));
  auto event = gui::CheckEvent();
  if (event & ~gui::kEventHover) {
    mouse_in_window_ = true;
    gui::ColorBackground(vec4(0.4f, 0.4f, 0.4f, 1));
  } else if (event & gui::kEventHover) {
    gui::ColorBackground(vec4(0.3f, 0.3f, 0.3f, 1));
  } else {
    gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 1));
  }
  gui::SetTextColor(vec4(1, 1, 1, 1));
  gui::Label(text, size);
  gui::EndGroup();  // $id
  return event;
}

}  // namespace editor
}  // namespace fpl
