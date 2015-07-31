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

#include "world_editor/flatbuffer_editor.h"
#include "flatbuffer_editor_config_generated.h"
#include "fplbase/utilities.h"
#include "fplbase/flatbuffer_utils.h"

namespace fpl {
namespace editor {

using flatbuffers::uoffset_t;

// Default UI layout, override in Flatbuffers.
static const int kDefaultUISize = 20;
static const int kDefaultUISpacing = 4;
static const int kDefaultBlankStringWidth = 10;
static const float kDefaultBGColor[] = {0, 0, 0, 1};
static const float kDefaultFGColor[] = {1, 1, 1, 1};

static void LoadColor(const ColorRGBA* color, const vec4& default_color,
                      mathfu::vec4* output) {
  *output = color ? LoadColorRGBA(color) : default_color;
}

FlatbufferEditor::FlatbufferEditor(const FlatbufferEditorConfig* config,
                                   const reflection::Schema& schema,
                                   const reflection::Object& table_def,
                                   const void* flatbuffer_data)
    : schema_(schema),
      table_def_(table_def),
      button_pressed_(kNone),
      show_types_(false),
      expand_all_(false),
      edit_fields_modified_(false),
      flatbuffer_modified_(false) {
  // By default, use our memory location as the FlatUI ID. If you want
  // to use something more human-friendly, call set_root_id.
  root_id_ = std::string("fbedit:") +
             flatbuffers::NumToString(reinterpret_cast<size_t>(this));
  config_read_only_ = config->read_only();
  config_auto_commit_ = config->auto_commit_edits();
  if (flatbuffer_data != nullptr) {
    CopyTable(flatbuffer_data, &flatbuffer_);
  }
  const vec4 default_bg(kDefaultBGColor);
  const vec4 default_fg(kDefaultFGColor);
  if (config == nullptr) {
    // Default colors.
    LogInfo("Color defaults.");
    ui_size_ = kDefaultUISize;
    ui_spacing_ = kDefaultUISpacing;
    blank_field_width_ = kDefaultBlankStringWidth;
    bg_button_color_ = default_bg;
    bg_button_hover_color_ = default_bg;
    bg_button_click_color_ = default_bg;
    text_disabled_color_ = default_fg;
    text_normal_color_ = default_fg;
    text_button_color_ = default_fg;
    text_editable_color_ = default_fg;
    text_modified_color_ = default_fg;
    text_error_color_ = default_fg;
  } else {
    ui_size_ = config->ui_size();
    ui_spacing_ = config->ui_spacing();
    blank_field_width_ = config->blank_field_width();
    LoadColor(config->bg_button_color(), default_bg, &bg_button_color_);
    LoadColor(config->bg_button_hover_color(), default_bg,
              &bg_button_hover_color_);
    LoadColor(config->bg_button_click_color(), default_bg,
              &bg_button_click_color_);

    LoadColor(config->text_normal_color(), default_fg, &text_normal_color_);
    LoadColor(config->text_button_color(), default_fg, &text_button_color_);
    LoadColor(config->text_disabled_color(), default_fg, &text_disabled_color_);
    LoadColor(config->text_editable_color(), default_fg, &text_editable_color_);
    LoadColor(config->text_modified_color(), default_fg, &text_modified_color_);
    LoadColor(config->text_error_color(), default_fg, &text_error_color_);
  }
}

gui::Event FlatbufferEditor::TextButton(const char* text, const char* id,
                                        int size) {
  const float kMargin = 1;
  float text_size = size - 2 * kMargin;
  gui::StartGroup(gui::kLayoutHorizontalTop, 0, id);
  gui::SetMargin(gui::Margin(kMargin));
  auto event = gui::CheckEvent();
  if (event & ~gui::kEventHover) {
    gui::ColorBackground(bg_button_click_color_);
  } else if (event & gui::kEventHover) {
    gui::ColorBackground(bg_button_hover_color_);
  } else {
    gui::ColorBackground(bg_button_color_);
  }
  gui::SetTextColor(text_button_color_);
  gui::Label(text, text_size);
  gui::EndGroup();  // id
  return event;
}

bool FlatbufferEditor::AddFieldButton(VisitMode mode, const std::string& name,
                                      const std::string& typestr,
                                      const std::string& id) {
  if (mode == kCommitEdits && force_commit_field_ == id) {
    return true;
  }
  if (IsDraw(mode)) {
    gui::StartGroup(gui::kLayoutHorizontalCenter, ui_spacing(),
                    (id + "-container").c_str());
    gui::Label((name + ": ").c_str(), ui_size());
    if (IsDrawEdit(mode)) {
      if (TextButton(("[add " + typestr + "]").c_str(),
                     (id + "-addField").c_str(), ui_size()) &
          gui::kEventWentUp) {
        force_commit_field_ = id;
        edit_fields_modified_ = true;
      }
    }
    gui::EndGroup();  // id + "-container"
  }
  return false;
}

void FlatbufferEditor::Update() {
  if (force_commit_field_ != "" || button_pressed_ == kCommit) {
    CommitEditsToFlatbuffer();
  } else if (button_pressed_ == kRevert) {
    ClearEditFields();
  }
  button_pressed_ = kNone;
  force_commit_field_ = "";
}

void FlatbufferEditor::Draw() {
  set_keyboard_in_use(false);
  if (HasFlatbufferData()) {
    edit_fields_modified_ = false;
    VisitFlatbufferTable(kCheckEdits, schema_, table_def_,
                         *flatbuffers::GetAnyRoot(flatbuffer_.data()),
                         root_id());
    gui::StartGroup(gui::kLayoutVerticalLeft, ui_spacing(),
                    (root_id() + "-contents").c_str());
    if (!config_auto_commit_ && edit_fields_modified_) {
      // Draw some buttons allowing us to commit or revert all edits to this FB.
      gui::StartGroup(gui::kLayoutHorizontalTop, ui_spacing(),
                      (root_id() + "-buttons").c_str());
      if (TextButton("[Apply All]", (root_id() + "-button-commit").c_str(),
                     ui_size()) &
          gui::kEventWentUp) {
        button_pressed_ = kCommit;
      }
      if (TextButton("[Revert All]", (root_id() + "-button-commit").c_str(),
                     ui_size()) &
          gui::kEventWentUp) {
        button_pressed_ = kRevert;
      }
      gui::EndGroup();  // root_id + "-buttons"
    }
    std::string previously_editing_field = currently_editing_field_;
    currently_editing_field_ = "";

    VisitFlatbufferTable(
        config_read_only_
            ? kDrawReadOnly
            : (config_auto_commit_ ? kDrawEditAuto : kDrawEditManual),
        schema_, table_def_, *flatbuffers::GetAnyRoot(flatbuffer_.data()),
        root_id());

    // If we were previously editing a field and no longer are, commit the new
    // value to the FlatBuffer.
    if (previously_editing_field != "" && currently_editing_field_ == "" &&
        edit_fields_modified_) {
      force_commit_field_ = previously_editing_field;
    }

    gui::EndGroup();  // root_id + "-contents"
  }
}

void FlatbufferEditor::ClearFlatbufferModifiedFlag() {
  flatbuffer_modified_ = false;
  committed_fields_.clear();
}

bool FlatbufferEditor::GetFlatbufferCopy(
    std::vector<uint8_t>* flatbuffer_output) {
  if (flatbuffer_.size() != 0) {
    CopyTable(flatbuffer_.data(), flatbuffer_output);
    return true;
  } else {
    // No data to return.
    return false;
  }
}

bool FlatbufferEditor::GetFlatbufferCopy(std::string* flatbuffer_output) {
  if (flatbuffer_.size() != 0) {
    std::vector<uint8_t> buf;
    GetFlatbufferCopy(&buf);
    *flatbuffer_output =
        std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
    return true;
  } else {
    // No data to return.
    return false;
  }
}

std::unique_ptr<uint8_t> FlatbufferEditor::GetFlatbufferCopy() {
  if (flatbuffer_.size() != 0) {
    std::vector<uint8_t> buf;
    GetFlatbufferCopy(&buf);
    std::unique_ptr<uint8_t> ptr;
    ptr.reset(new uint8_t[buf.size()]);
    memcpy(ptr.get(), buf.data(), buf.size());
    return ptr;
  } else {
    // No data to return.
    return nullptr;
  }
}

void FlatbufferEditor::CopyTable(const void* src, std::vector<uint8_t>* dest) {
  flatbuffers::FlatBufferBuilder fbb;
  auto table = flatbuffers::CopyTable(
      fbb, schema_, table_def_,
      *flatbuffers::GetAnyRoot(reinterpret_cast<const uint8_t*>(src)));
  fbb.Finish(table);
  *dest = {fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize()};
}

void FlatbufferEditor::CommitEditsToFlatbuffer() {
  bool go_again;
  do {
    go_again = VisitFlatbufferTable(
        kCommitEdits, schema_, table_def_,
        *flatbuffers::GetAnyRoot(flatbuffer_.data()), root_id());
    // VisitFlatbufferTable(kCommitEdits, ...) returns true if
    // something in the Flatbuffer needed to be resized, in which case
    // it has stopped committing edits right after that point, and you
    // need to run it again to resume (and continue to do so until it
    // returns false).
  } while (go_again);
  edit_fields_modified_ = false;
}

// Begin helper functions for inputting/outputting Flatbuffer data

static const char kStructSep[] = ", ";
static const char kStructBegin[] = "< ";
static const char kStructEnd[] = " >";

std::string FlatbufferEditor::ExtractInlineStructDef(const std::string& str) {
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

static std::string ConsumeCommasAndWhitespace(const std::string& str) {
  // Strip away commas and whitespace from the start of a string.
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != ',' && str[i] != ' ') {
      return str.substr(i);
    }
  }
  return "";  // Everything was consumed!
}

static std::string ConsumeWhitespace(const std::string& str) {
  // Strip away whitespace from the start of a string.
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != ' ') {
      return str.substr(i);
    }
  }
  return "";  // Everything was consumed!
}

static std::string ConsumeNumber(const std::string& str) {
  // Strip away an optional leading hypen, multiple digits, and up to
  // one decimal point from the start of a string.
  bool got_decimal = false;
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      // Not a digit.
      if (str[i] == '.' && !got_decimal) {
        // Allow a single decimal point.
        got_decimal = true;
      } else if (i == 0 && str[i] == '-') {
        // Leading unary -, okay to keep.
      } else {
        return str.substr(i);
      }
    }
  }
  return "";  // Everything was consumed!
}

bool FlatbufferEditor::ParseStringIntoStruct(
    const std::string& struct_def, const reflection::Schema& schema,
    const reflection::Object& objectdef, flatbuffers::Struct* struct_ptr) {
  std::string str = ExtractInlineStructDef(struct_def);
  if (str.length() == 0) {
    LogInfo("Struct parse error: overall struct def");
    return false;
  }

  for (auto sit = objectdef.fields()->begin(); sit != objectdef.fields()->end();
       ++sit) {
    str = ConsumeWhitespace(str);
    const reflection::Field& fielddef = **sit;
    switch (fielddef.type()->base_type()) {
      default: {
        // Scalar value.
        std::string new_str = ConsumeNumber(str);
        if (new_str == str) {
          LogInfo("Struct parse error: scalar. str = '%s'", str.c_str());
          return false;
        }
        if (struct_ptr != nullptr)
          flatbuffers::SetAnyFieldS(struct_ptr, fielddef, str.c_str());
        str = new_str;
        break;
      }
      case reflection::Obj: {
        // Sub-struct inside this struct.
        auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());
        if (subobjdef.is_struct()) {
          std::string substr = ExtractInlineStructDef(str);
          if (substr.length() == 0) {
            LogInfo("Struct parse error: extracting substruct. str = '%s'",
                    str.c_str());
            return false;
          }
          flatbuffers::Struct* sub_struct =
              struct_ptr
                  ? flatbuffers::GetAnyFieldAddressOf<flatbuffers::Struct>(
                        *struct_ptr, fielddef)
                  : nullptr;
          if (!ParseStringIntoStruct(substr, schema, subobjdef, sub_struct)) {
            LogInfo("Struct parse error: substruct");
            return false;
          }
          str = str.substr(substr.length());
        }
      }
    }
    str = ConsumeCommasAndWhitespace(str);
    // Are we out of string?
    if (str.length() == 0) break;
  }
  return true;
}

std::string FlatbufferEditor::StructToString(
    const reflection::Schema& schema, const reflection::Object& objectdef,
    const flatbuffers::Struct& fbstruct, bool field_names_only) {
  std::string output = kStructBegin;
  for (auto sit = objectdef.fields()->begin(); sit != objectdef.fields()->end();
       ++sit) {
    if (sit != objectdef.fields()->begin()) {
      output += kStructSep;
    }
    const reflection::Field& fielddef = **sit;
    switch (fielddef.type()->base_type()) {
      case reflection::Obj: {
        auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());
        // It must be a struct, because structs can only contain scalars and
        // other structs, but let's just make sure.
        if (subobjdef.is_struct()) {
          const flatbuffers::Struct* sub_struct =
              flatbuffers::GetAnyFieldAddressOf<const flatbuffers::Struct>(
                  fbstruct, fielddef);
          if (field_names_only) {
            output += fielddef.name()->str() + ":";
          }
          output +=
              StructToString(schema, subobjdef, *sub_struct, field_names_only);
        }
        break;
      }
      default: {
        // Scalar value inside the struct.
        if (field_names_only) {
          output += fielddef.name()->str();
        } else {
          output += flatbuffers::GetAnyFieldS(fbstruct, fielddef);
        }
        break;
      }
    }
  }
  output += kStructEnd;
  return output;
}

std::string FlatbufferEditor::FormatFieldName(const std::string& name,
                                              const std::string& type) {
  return (type != "" && show_types()) ? (name + "<" + type + ">: ")
                                      : (name + ": ");
}

bool FlatbufferEditor::VisitField(VisitMode mode, const std::string& name,
                                  const std::string& value,
                                  const std::string& type,
                                  const std::string& comment,
                                  const std::string& id) {
  if (mode != kDrawReadOnly) {
    if (edit_fields_.find(id) == edit_fields_.end()) {
      edit_fields_[id] = value;
    }
  }
  std::string edit_id = id + "-edit";
  if (IsDraw(mode)) {
    gui::StartGroup(gui::kLayoutHorizontalCenter, ui_spacing(),
                    (id + "-container").c_str());
    gui::Label(FormatFieldName(name, type).c_str(), ui_size());
  }
  if (mode == kDrawReadOnly) {
    // Show in "read-only" color.
    gui::SetTextColor(text_disabled_color_);
  } else {
    if (edit_fields_[id] != value) {
      // Show in "modified" color.
      if (IsDrawEdit(mode)) gui::SetTextColor(text_modified_color_);
      edit_fields_modified_ = true;
      if (mode == kCommitEdits &&
          (force_commit_field_ == "" || force_commit_field_ == id)) {
        LogInfo("VisitField: Setting '%s' to '%s' (was: '%s')", id.c_str(),
                edit_fields_[id].c_str(), value.c_str());
        committed_fields_.insert(id);
        return true;  // Return true if the field was changed.
      }
    } else {
      // Show in "editable" but not modified color.
      if (IsDrawEdit(mode)) gui::SetTextColor(text_editable_color_);
    }
  }
  if (IsDrawEdit(mode)) {
    vec2 edit_vec = vec2(0, 0);
    if (edit_fields_[id].length() == 0) {
      edit_vec.x() = blank_field_width();
    }
    if (gui::Edit(ui_size(), edit_vec, edit_id.c_str(), &edit_fields_[id])) {
      if (mode == kDrawEditAuto) {
        // In this mode, we track which field the user is editing; once they
        // stop editing it, we auto-commit.
        currently_editing_field_ = id;
      }
      set_keyboard_in_use(true);
    }
  } else if (mode == kDrawReadOnly) {
    gui::Label(value.c_str(), ui_size());
  }
  if (mode == kDrawEditManual) {
    // In this mode, the user must commit each field they edit.
    if (edit_fields_[id] != value) {
      if (TextButton("[apply]", (id + "-apply").c_str(), ui_size()) &
          gui::kEventWentUp) {
        force_commit_field_ = id;
      }
      if (TextButton("[revert]", (id + "-revert").c_str(), ui_size()) &
          gui::kEventWentUp) {
        edit_fields_[id] = value;
      }
    }
  }
  if (IsDraw(mode)) {
    gui::SetTextColor(text_normal_color_);
    if (comment != "") {
      gui::Label(comment.c_str(), ui_size());
    }
    gui::EndGroup();  // id + "-container"
  }
  return false;  // Nothing changed while visiting this field.
}

bool FlatbufferEditor::VisitSubtable(VisitMode mode, const std::string& field,
                                     const std::string& type,
                                     const std::string& comment,
                                     const std::string& id,
                                     const reflection::Schema& schema,
                                     const reflection::Object& subobjdef,
                                     flatbuffers::Table& subtable) {
  if (mode == kCommitEdits || mode == kCheckEdits || expand_all() ||
      expanded_subtables_.find(id) != expanded_subtables_.end()) {
    // Subtable which is not a struct (since VisitFlatbufferStruct would have
    // been called in that case).
    if (IsDraw(mode)) {
      gui::StartGroup(gui::kLayoutHorizontalTop, ui_spacing(),
                      (id + "-field").c_str());
      gui::StartGroup(gui::kLayoutVerticalLeft, ui_spacing(),
                      (id + "-fieldName").c_str());
      auto event = gui::CheckEvent();
      std::string name_full = field;
      gui::Label(FormatFieldName(field, type).c_str(), ui_size());
      if (event & gui::kEventWentDown) {
        if (!expand_all()) expanded_subtables_.erase(id);
      }
      gui::EndGroup();  // id + "-fieldName"
      gui::StartGroup(gui::kLayoutVerticalLeft, ui_spacing(),
                      (id + "-nestedTable").c_str());
    }
    if (VisitFlatbufferTable(mode, schema, subobjdef, subtable, id)) {
      return true;
    }
    if (IsDraw(mode)) {
      gui::EndGroup();  // id + "-nestedTable"
      if (comment != "") {
        gui::Label((std::string("(") + comment + ")").c_str(), ui_size());
      }
      gui::EndGroup();  // id + "-field"
    }
  } else if (IsDraw(mode)) {
    // Show this table collapsed.
    gui::StartGroup(gui::kLayoutHorizontalTop, ui_spacing(),
                    (id + "-field").c_str());
    auto event = gui::CheckEvent();
    if (event & gui::kEventWentDown) {
      expanded_subtables_.insert(id);
    }
    gui::StartGroup(gui::kLayoutHorizontalTop, ui_spacing(),
                    (id + "-fieldName").c_str());
    gui::Label(FormatFieldName(field, type).c_str(), ui_size());
    gui::EndGroup();  // id + "-fieldName"
    gui::StartGroup(gui::kLayoutVerticalLeft, ui_spacing(),
                    (id + "-nestedTable").c_str());
    gui::Label("...", ui_size());
    if (comment != "") {
      gui::Label((std::string("(") + comment + ")").c_str(), ui_size());
    }
    gui::EndGroup();  // id + "-nestedTable"
    gui::EndGroup();  // id + "-field"
  }
  return false;  // Nothing changed in the subtable.
}

bool FlatbufferEditor::VisitFlatbufferTable(VisitMode mode,
                                            const reflection::Schema& schema,
                                            const reflection::Object& objectdef,
                                            flatbuffers::Table& table,
                                            const std::string& id) {
  auto fielddefs = objectdef.fields();
  for (auto it = fielddefs->begin(); it != fielddefs->end(); ++it) {
    const reflection::Field& fielddef = **it;
    if (VisitFlatbufferField(mode, schema, fielddef, objectdef, table, id))
      return true;
  }
  return false;  // No resize occurred while visiting the table.
}

bool FlatbufferEditor::VisitFlatbufferField(VisitMode mode,
                                            const reflection::Schema& schema,
                                            const reflection::Field& fielddef,
                                            const reflection::Object& objectdef,
                                            flatbuffers::Table& table,
                                            const std::string& id) {
  std::string new_id = id + "." + fielddef.name()->str();
  auto base_type = fielddef.type()->base_type();
  switch (base_type) {
    default: {
      if (!table.CheckField(fielddef.offset())) {
        if (AddFieldButton(mode, fielddef.name()->str(), "scalar", new_id)) {
          // TODO: Add a scalar field here and then return true.
          return false;
        }
      } else {
        VisitFlatbufferScalar(mode, schema, fielddef, table, new_id);
        // Mutated scalars never resize, so we can ignore the "true" return
        // value.
      }
      break;
    }
    case reflection::String: {
      if (!table.CheckField(fielddef.offset())) {
        if (AddFieldButton(mode, fielddef.name()->str(), "string", new_id)) {
          // Add a string field here, then return true if we resized the array.
          flatbuffers::FlatBufferBuilder fbb;
          auto offset = fbb.CreateString("--blank--");
          fbb.Finish(offset);
          const uint8_t* new_data = flatbuffers::AddFlatBuffer(
              flatbuffer_, fbb.GetBufferPointer(), fbb.GetSize());
          if (!SetFieldT(&table, fielddef, new_data)) {
            LogError("Couldn't add new string value to Flatbuffer!");
          }
          flatbuffer_modified_ = true;
          return true;
        }
      } else {
        if (VisitFlatbufferString(mode, schema, fielddef, table, new_id))
          return true;  // Mutated strings may need to be resized
      }
      break;
    }
    case reflection::Obj: {
      auto& subobjdef = *schema.objects()->Get(fielddef.type()->index());

      if (subobjdef.is_struct()) {
        if (!table.CheckField(fielddef.offset())) {
          if (AddFieldButton(mode, fielddef.name()->str(), "struct", new_id)) {
            // TODO: Add a struct field here, then return true.
            return false;
          }
        } else {
          flatbuffers::Struct* struct_ptr =
              table.GetStruct<flatbuffers::Struct*>(fielddef.offset());
          VisitFlatbufferStruct(mode, schema, fielddef, subobjdef, *struct_ptr,
                                new_id);
          // Mutated structs never need to be resized
        }
      } else {
        if (!table.CheckField(fielddef.offset())) {
          if (AddFieldButton(mode, fielddef.name()->str(), "table", new_id)) {
            // TODO: Add a table field here, then return true.
            return false;
          }
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
      }
      break;
    }
    case reflection::Union: {
      if (!table.CheckField(fielddef.offset())) {
        if (AddFieldButton(mode, fielddef.name()->str(), "union", new_id)) {
          // TODO: Add a union field here, then return true.
          // (Hmm, how do we choose the type?)
          return false;
        }
      } else {
        if (VisitFlatbufferUnion(mode, schema, fielddef, objectdef, table,
                                 new_id)) {
          return true;  // Mutated unions may need to be resized
        }
      }
      break;
    }
    case reflection::Vector: {
      if (!table.CheckField(fielddef.offset())) {
        if (AddFieldButton(mode, fielddef.name()->str(), "vector", new_id)) {
          // TODO: Add a vector field here, then return true.
          return false;
        }
      } else {
        if (VisitFlatbufferVector(mode, schema, fielddef, table, new_id)) {
          return true;
        }
      }
      break;
    }
  }
  return false;  // No resize occurred while visiting this field.
}

// There are way faster ways to do popcount, but this one is simple.
static unsigned int NumBitsSet(uint64_t n) {
  unsigned int total = 0;
  for (uint64_t i = 0; i < 8 * sizeof(uint64_t); i++) {
    if (n & (1L << i)) total++;
  }
  return total;
}

std::string FlatbufferEditor::GetEnumTypeAndValue(
    const reflection::Schema& schema, const reflection::Field& fielddef,
    const std::string& value, std::string* type, std::string* value_name) {
  *type = "";
  *value_name = "";
  std::string scalar_value = value;
  if (fielddef.type()->index() >= 0) {
    // You're an enum, Harry!
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
          // No value for 0 - show that the bit flags are blank.
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
            // Bits were left over, so that doesn't work for us.
            *value_name += " | ???";
          }
        }
      } else {
        // Couldn't find it as a bit flag value.
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

bool FlatbufferEditor::VisitFlatbufferScalar(VisitMode mode,
                                             const reflection::Schema& schema,
                                             const reflection::Field& fielddef,
                                             flatbuffers::Table& table,
                                             const std::string& id) {
  // Check for enum...
  std::string value, enum_value, type, comment;
  enum_value = value = GetAnyFieldS(table, fielddef, &schema);
  if (mode != kDrawReadOnly && edit_fields_.find(id) != edit_fields_.end()) {
    // If we've typed in a new value, show the enum value for what we've typed.
    enum_value = edit_fields_[id].c_str();
  }
  GetEnumTypeAndValue(schema, fielddef, enum_value, &type, &comment);

  if (VisitField(mode, fielddef.name()->str(), value, type, comment, id)) {
    // Read the value of edit_fields_[id] and put it into the current table.
    SetAnyFieldS(&table, fielddef, edit_fields_[id].c_str());
    flatbuffer_modified_ = true;
  }
  return false;  // Mutated scalars will never need to resize.
}

bool FlatbufferEditor::VisitFlatbufferString(VisitMode mode,
                                             const reflection::Schema& schema,
                                             const reflection::Field& fielddef,
                                             flatbuffers::Table& table,
                                             const std::string& id) {
  const flatbuffers::String* str;
  std::string str_text, comment;
  if (fielddef.offset() == 0)
    comment = "(no value)";
  else {
    str = GetFieldS(table, fielddef);
    str_text = str->str();
  }

  if (VisitField(mode, fielddef.name()->str(), str_text, "string", comment,
                 id)) {
    // Read the value of edit_fields_[id] and put it into the current table.
    // TODO: Remove the const_cast when SetString no longer requires it.
    SetString(schema, edit_fields_[id], const_cast<flatbuffers::String*>(str),
              &flatbuffer_, &table_def_);
    flatbuffer_modified_ = true;
    return true;
  }
  return false;
}

// TODO: Add an option to expand structs rather than edit them in-line in a
// single string.
bool FlatbufferEditor::VisitFlatbufferStruct(
    VisitMode mode, const reflection::Schema& schema,
    const reflection::Field& fielddef, const reflection::Object& objectdef,
    flatbuffers::Struct& fbstruct, const std::string& id) {
  if (VisitField(
          mode, fielddef.name()->str(),
          StructToString(schema, objectdef, fbstruct, false),
          objectdef.name()->str(),
          show_types() ? StructToString(schema, objectdef, fbstruct, true) : "",
          id)) {
    LogInfo("Struct %s edited, new value is %s", id.c_str(),
            edit_fields_[id].c_str());
    // Check if the field is valid first.
    bool valid =
        ParseStringIntoStruct(edit_fields_[id], schema, objectdef, nullptr);
    if (valid) {
      LogInfo("Struct '%s' WAS valid for %s.", edit_fields_[id].c_str(),
              id.c_str());
      ParseStringIntoStruct(edit_fields_[id], schema, objectdef, &fbstruct);
      flatbuffer_modified_ = true;
    } else {
      LogInfo("Struct '%s' was not valid for %s.", edit_fields_[id].c_str(),
              id.c_str());
      // TODO: mark invalid fields in red.
    }
  }
  return false;
}

bool FlatbufferEditor::VisitFlatbufferUnion(VisitMode mode,
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

bool FlatbufferEditor::VisitFlatbufferVector(VisitMode mode,
                                             const reflection::Schema& schema,
                                             const reflection::Field& fielddef,
                                             flatbuffers::Table& table,
                                             const std::string& id) {
  if (fielddef.offset() == 0) return false;
  flatbuffers::VectorOfAny* vec = GetFieldAnyV(table, fielddef);
  auto element_base_type = fielddef.type()->element();
  auto elemobjectdef = element_base_type == reflection::Obj
                           ? schema.objects()->Get(fielddef.type()->index())
                           : nullptr;
  uoffset_t element_size = flatbuffers::GetTypeSizeInline(
      element_base_type, fielddef.type()->index(), schema);
  std::string idx = ".size";
  if (IsDraw(mode))
    gui::StartGroup(gui::kLayoutHorizontalCenter, 8,
                    (id + idx + "-commit").c_str());
  // Special: Don't auto-commit vector sizes back to the Flatbuffer as it causes
  // too much to change; if VisitMode is kDrawEditAuto, pass in kDrawEditManual.
  if (VisitField(mode == kDrawEditAuto ? kDrawEditManual : mode,
                 fielddef.name()->str() + idx,
                 flatbuffers::NumToString(vec->size()), "size_t", "",
                 id + idx)) {
    uoffset_t new_size =
        flatbuffers::StringToInt(edit_fields_[id + idx].c_str());
    flatbuffers::ResizeAnyVector(schema, new_size, vec, vec->size(),
                                 element_size, &flatbuffer_, &table_def_);
    if (IsDraw(mode)) gui::EndGroup();  // id + idx + "-commit"
    return true;
  }
  if (IsDraw(mode)) gui::EndGroup();  // id + idx + "-commit"
  switch (element_base_type) {
    case reflection::String: {
      // This is a vector of strings.
      for (size_t i = 0; i < vec->size(); i++) {
        std::string idx = "[" + flatbuffers::NumToString(i) + "]";
        flatbuffers::String* str =
            flatbuffers::GetAnyVectorElemPointer<flatbuffers::String>(vec, i);
        if (VisitField(mode, fielddef.name()->str() + idx, str->str(), "string",
                       "", id + idx)) {
          SetString(schema, edit_fields_[id + idx], str, &flatbuffer_,
                    &table_def_);
          flatbuffer_modified_ = true;
          return true;
        }
      }
      break;
    }
    case reflection::Obj: {
      if (!elemobjectdef->is_struct()) {
        // This is a vector of tables.
        for (uoffset_t i = 0; i < vec->size(); i++) {
          std::string idx = "[" + flatbuffers::NumToString(i) + "]";
          flatbuffers::Table* tableelem =
              flatbuffers::GetAnyVectorElemPointer<flatbuffers::Table>(vec, i);
          if (VisitSubtable(mode, fielddef.name()->str() + idx,
                            elemobjectdef->name()->str(), "", id + idx, schema,
                            *elemobjectdef, *tableelem))
            return true;  // Mutated tables may require resize.
        }
      } else {
        // This is a vector of structs.
        for (uoffset_t i = 0; i < vec->size(); i++) {
          std::string idx = "[" + flatbuffers::NumToString(i) + "]";
          flatbuffers::Struct* struct_ptr =
              flatbuffers::GetAnyVectorElemAddressOf<flatbuffers::Struct>(
                  vec, i, element_size);
          VisitFlatbufferStruct(mode, schema, fielddef, *elemobjectdef,
                                *struct_ptr, id + idx);
        }
      }
      break;
    }
    default: {
      // This is a vector of scalars.
      std::string output;
      for (uoffset_t i = 0; i < vec->size(); i++) {
        std::string idx = "[" + flatbuffers::NumToString(i) + "]";
        std::string enum_type, enum_hint;
        std::string value =
            flatbuffers::GetAnyVectorElemS(vec, element_base_type, i);
        if (edit_fields_.find(id + idx) != edit_fields_.end()) {
          value = edit_fields_[id + idx];
        }
        value = GetEnumTypeAndValue(schema, fielddef, value, &enum_type,
                                    &enum_hint);

        if (VisitField(mode, fielddef.name()->str() + idx, value, enum_type,
                       enum_hint, id + idx)) {
          // Handle the same way as VisitFlatbufferScalar does.
          flatbuffers::SetAnyVectorElemS(vec, element_base_type, i,
                                         edit_fields_[id + idx].c_str());
          flatbuffer_modified_ = true;
        }
      }
      break;
    }
  }
  return false;
}

}  // namespace editor
}  // namespace fpl
