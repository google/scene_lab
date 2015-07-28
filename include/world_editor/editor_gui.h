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

#ifndef FPL_EDITOR_GUI_H_
#define FPL_EDITOR_GUI_H_

#include <memory>
#include <set>
#include <string>
#include <string.h>
#include <unordered_map>
#include "component_library/entity_factory.h"
#include "entity/entity_manager.h"
#include "event/event_manager.h"
#include "flatui/flatui.h"
#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "world_editor_config_generated.h"

namespace fpl {
namespace editor {

class EditorGui : public event::EventListener {
 public:
  EditorGui(const WorldEditorConfig* config,
            entity::EntityManager* entity_manager, FontManager* font_manager,
            const std::string* schema_data);
  virtual void OnEvent(const event::EventPayload& event_payload);

  void Render();

  bool InputCaptured() const { return mouse_in_window() || keyboard_in_use(); }

  bool mouse_in_window() const { return mouse_in_window_; }
  bool keyboard_in_use() const { return keyboard_in_use_; }

  void SetEditEntity(entity::EntityRef& entity);
  entity::EntityRef edit_entity() const { return edit_entity_; }

  // Call this if you change any entity data externally
  void ClearEntityData() { entity_data_.clear(); }
  // Write the entity fields that were changed to the entity_data_ flatbuffer,
  // then import the changed flatbuffers back to the edit_entity_.
  void CommitEntityData();

  // Should we let the editor deselect this as the current entity?
  // Probably not if we've made any changes.
  bool CanDeselectEntity() const { return !EntityModified(); }

  // Has the entity been modified in any way? Either the on-screen fields
  // or the in-memory flatbuffer?
  bool EntityModified() const {
    return EntityFieldsModified() || EntityBufferModified();
  }
  // Have the on-screen fields been modified, but not pushed to the Flatbuffer?
  bool EntityFieldsModified() const {
    for (size_t i = 0; i < component_fields_modified_.size(); i++) {
      if (component_fields_modified_[i]) return true;
    }
    return false;
  }
  // Has the entity flatbuffer been modified, but not imported into the entity?
  bool EntityBufferModified() const {
    for (size_t i = 0; i < component_buffer_modified_.size(); i++) {
      if (component_buffer_modified_[i]) return true;
    }
    return false;
  }

  // Does the user want you to show the current entity's physics?
  bool show_physics() const { return show_physics_; }

 private:
  enum VisitMode { kDraw, kDrawReadOnly, kEdit };
  enum WindowState { kNormal, kMaximized, kHidden };

  static const int kVirtualResolution = 1000;
  static const int kToolbarHeight = 30;
  static const int kSpacing = 3;
  static const int kIndent = 3;
  static const int kFontSize = 18;
  static const int kBlankStringEditWidth = 8;

  // Read the raw data from a given entity's component and save it into
  // a vector, so we can mutate it later.
  bool ReadDataFromEntity(const entity::EntityRef& entity,
                          entity::ComponentId component,
                          std::vector<uint8_t>* output_vector) const;

  // Propagate all of the edited fields from edit_fields_ to entity_data_ for
  // the given component. Sets component_buffer_modified_ to true if there was
  // anything propagated, and component_fields_modified_ to false once finished
  // propagating. Returns true if the entity_data_ buffer for this component was
  // changed.
  bool PropagateEdits(entity::ComponentId id);

  // Gui helper function to capture mouse clicks and prevent the world editor
  // from consuming them.
  void CaptureMouseClicks();

  // Draw an interface for editing an entity.
  void DrawEditEntityUI();
  // Draw a list of all of the component data that this entity has.
  void DrawEntityComponent(entity::ComponentId id);
  // Draw a list of the entity's parent and children, if any.
  void DrawEntityFamily();

  // Draw the flatbuffer schema, traversing it via reflection and displaying
  // it via FlatUI.
  // Return true if the flatbuffer was drawn, false if there was an error
  // reading the schema data.
  bool DrawFlatbuffer(uint8_t* data, const char* table_name);

  // Go through the flatbuffer, grabbing edited fields from edit_fields_
  // as it finds them, and placing them into the flatbuffer.
  // Returns true if the flatbuffer needed to be resized as part of being
  // edited, which means not all the edits made it through and you'll need
  // to call this again and again until it returns false.
  bool EditFlatbuffer(std::vector<uint8_t>* data, const char* table_name);

  // Visit a single field with the given name and value. The "id" should
  // uniquely identify it in the tree of data structures.
  //
  // If mode is kDraw, the Flatbuffer field will be drawn using a FlatUI Edit()
  // control. kDrawReadOnly is similar, but it will use a Label() control and
  // won't be editable.
  //
  // If mode is kEdit, it won't actually draw, but it apply any edits made from
  // the previously visited fields, hitting all of the previously calculated IDs
  // to apply the edits.
  //
  // Only returns true if mode = kEdit and if we applied any edits.
  bool VisitField(VisitMode mode, const std::string& name,
                  const std::string& value, const std::string& type,
                  const std::string& comment, const std::string& id);

  // Visit a subtable with the given name. The "id" should uniquely identify it
  // in the tree of data structures. If mode is kEdit, don't actually draw,
  // but do still propagate through the tree of data structures so we can apply
  // edits to the previously determined IDs. Returns true if kEdit and if we
  // applied any edits.
  bool VisitSubtable(VisitMode mode, const std::string& field,
                     const std::string& type, const std::string& comment,
                     const std::string& id, const reflection::Schema& schema,
                     const reflection::Object& subobjdef,
                     flatbuffers::Table& subtable);

  // Helper functions for traversing flatbuffers via reflection.
  // They may only return true if mode = kEdit and a field was edited in such a
  // way as to force the flatbuffer to be resized, which means you'll need to
  // start over and propagate edits again to catch the subsequent edits.
  bool VisitFlatbufferField(VisitMode mode, const reflection::Schema& schema,
                            const reflection::Field& fielddef,
                            const reflection::Object& objectdef,
                            flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferScalar(VisitMode mode, const reflection::Schema& schema,
                             const reflection::Field& fielddef,
                             flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferTable(VisitMode mode, const reflection::Schema& schema,
                            const reflection::Object& objectdef,
                            flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferVector(VisitMode mode, const reflection::Schema& schema,
                             const reflection::Field& fielddef,
                             flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferUnion(VisitMode mode, const reflection::Schema& schema,
                            const reflection::Field& fielddef,
                            const reflection::Object& objectdef,
                            flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferStruct(VisitMode mode, const reflection::Schema& schema,
                             const reflection::Field& fielddef,
                             const reflection::Object& objectdef,
                             flatbuffers::Table& table, const std::string& id);
  bool VisitFlatbufferString(VisitMode mode, const reflection::Schema& schema,
                             const reflection::Field& fielddef,
                             flatbuffers::Table& table, const std::string& id);

  // Create a text button; call this inside a gui::Run.
  gui::Event TextButton(const char* text, const char* id, int size);

  // Show a button that, if you click on it, selects another entity.
  bool EntityButton(entity::EntityRef& entity, int size);

  // Get the string representation of a value of the given type at a
  // given memory location.
  static std::string GetScalarAsString(reflection::BaseType base_type,
                                       const void* ptr);
  // Get the string representation of a Flatbuffers struct at a given pointer
  // location. For example a Vec3 with x = 1.2, y = 3.4, z = 5 would show up as
  // < 1.2, 3.4, 5 >. Set field_names_only = true to output the field names
  // instead.
  static std::string StructToString(const reflection::Schema& schema,
                                    const reflection::Object& objectdef,
                                    const void* struct_ptr,
                                    bool field_names_only);

  // Parse a string that specifies a FlatBuffers struct in the format outputted
  // above. The format is "< 1, 2, < 3.4, 5, 6.7 >, 8 >". Each number must have
  // some combination of whitespace, comma, or angle brackets around it.
  // If you call this with a null struct_ptr it will just check whether your
  // string parses correctly.
  static bool ParseStringIntoStruct(const std::string& string,
                                    const reflection::Schema& schema,
                                    const reflection::Object& objectdef,
                                    void* struct_ptr);

  // Read an integer of the given type from the given string, and write out the
  // number of characters consumed to *chars_consumed.
  template <typename T>
  static T ReadInteger(const std::string& str, size_t* chars_consumed) {
    return static_cast<T>(ReadInt64(str, chars_consumed));
  }
  // Read a 64-bit integer from the given string, and write out the number of
  // characters consumed to *chars_consumed.
  static int64_t ReadInt64(const std::string& str, size_t* chars_consumed);

  // Read a floating point number of the given type from the given string, and
  // write out the number of characters consumed to *chars_consumed.
  template <typename T>
  static T ReadFloatingPoint(const std::string& str, size_t* chars_consumed) {
    return static_cast<float>(ReadDouble(str, chars_consumed));
  }

  // Read a double given type from the given string, and write out the number of
  // characters consumed to *chars_consumed.
  static double ReadDouble(const std::string& str, size_t* chars_consumed);

  // Parse a scalar value from the front of a larger string. Returns the
  // remainder of the string after the scalar number was extracted.
  static std::string ParseScalar(const std::string& str,
                                 reflection::BaseType base_type, void* output);
  // Extract a struct
  // str is a string that starts with '<'. Returns the string in between that
  // '<' and the matching '>' (exclusive), or empty string if there is a
  // mismatch.
  static std::string ExtractStructDef(const std::string& str);

  // If this scalar value is an enum, get its type name and the name of its
  // value. Returns a string with the corrected integer value after parsing.
  static std::string GetEnumTypeAndValue(const reflection::Schema& schema,
                                         const reflection::Field& fielddef,
                                         const std::string& value,
                                         std::string* type,
                                         std::string* value_name);

  // Get the virtual resolution (for FlatUI) of the whole screen.
  void GetVirtualResolution(vec2* resolution_output);

  // Clear the "entity modified" fields entirely.
  void ClearEntityModified() {
    ClearEntityFieldsModified();
    ClearEntityBufferModified();
  }
  void ClearEntityFieldsModified() {
    component_fields_modified_.resize(0);
    component_fields_modified_.resize(entity::kMaxComponentCount, 0);
  }
  void ClearEntityBufferModified() {
    component_buffer_modified_.resize(0);
    component_buffer_modified_.resize(entity::kMaxComponentCount, 0);
  }

  const WorldEditorConfig* config_;
  entity::EntityManager* entity_manager_;
  FontManager* font_manager_;
  const std::string* schema_data_;

  AssetManager* asset_manager_;
  component_library::EntityFactory* entity_factory_;
  event::EventManager* event_manager_;
  InputSystem* input_system_;
  Renderer* renderer_;

  // Which entity we are editing right now.
  entity::EntityRef edit_entity_;
  // We're changing which entity to select; this will take effect at the
  // end of rendering.
  entity::EntityRef changed_edit_entity_;

  // Temporary holding place for entity data until it's added into the entity.
  std::unordered_map<entity::ComponentId, std::vector<uint8_t>> entity_data_;
  // Pointer to whichever flatbuffer we are currently traversing, for resizing
  // purposes. Its bytes live in entity_data_.
  std::vector<uint8_t>* resizable_flatbuffer_;

  // List of table names we have expanded the view for.
  std::set<std::string> expanded_subtables_;

  std::vector<bool> components_to_show_;  // Components to display on screen.
  std::vector<bool> component_fields_modified_;  // Text fields were modified.
  std::vector<bool> component_buffer_modified_;  // Actual data was modified.
  entity::ComponentId
      component_id_visiting_;            // Component we are currently visiting.
  entity::ComponentId force_propagate_;  // Force this component's UI fields
                                         // into its buffer.
  // temporary holding place for edited fields until they go into entity_data_.
  std::unordered_map<std::string, std::string> edit_fields_;

  // TODO: Read these from config.
  mathfu::vec4 bg_edit_ui_color_;
  mathfu::vec4 bg_button_bar_color_;
  mathfu::vec4 bg_button_color_;
  mathfu::vec4 bg_hover_color_;
  mathfu::vec4 bg_click_color_;

  mathfu::vec4 text_button_color_;
  mathfu::vec4 text_normal_color_;
  mathfu::vec4 text_disabled_color_;
  mathfu::vec4 text_editable_color_;
  mathfu::vec4 text_modified_color_;
  mathfu::vec4 text_error_color_;

  mathfu::vec2i scroll_offset_;
  mathfu::vec2 virtual_resolution_;
  float edit_width_;
  WindowState edit_window_state_;
  bool show_physics_;
  bool show_types_;
  bool expand_all_;
  bool mouse_in_window_;
  bool keyboard_in_use_;
};

}  // namespace editor
}  // namespace fpl

#endif  // FPL_EDITOR_GUI_H_
