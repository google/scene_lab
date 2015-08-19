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
#include "world_editor/flatbuffer_editor.h"

namespace fpl {
namespace editor {

class EditorGui : public event::EventListener {
 public:
  EditorGui(const WorldEditorConfig* config,
            entity::EntityManager* entity_manager, FontManager* font_manager,
            const std::string* schema_data);
  virtual void OnEvent(const event::EventPayload& event_payload);

  // Render the game, then update based on button presses. You must either call
  // this, or call DrawGui() in a FlatUI Run context, followed by
  // UpdateAfterRender().
  void Render();

  // Either call Render() above, or call the following sequence if you are using
  // FlatUI elsewhere:
  // - StartRender();
  // - gui::Run(() { ... other Gui... DrawGui(...) };
  // - FinishRender();
  void StartRender();
  void DrawGui(const mathfu::vec2& virtual_resolution);
  void FinishRender();

  bool InputCaptured() const { return mouse_in_window() || keyboard_in_use(); }

  bool mouse_in_window() const { return mouse_in_window_; }
  bool keyboard_in_use() const { return keyboard_in_use_; }

  void SetEditEntity(entity::EntityRef& entity);
  entity::EntityRef edit_entity() const { return edit_entity_; }

  // Call this if you change any entity data externally, so we can reload
  // data directly from the entity.
  void ClearEntityData() { component_guis_.clear(); }
  // Write the entity fields that were changed to the entity_data_ flatbuffer,
  // then import the changed flatbuffers back to the edit_entity_.
  void CommitEntityData();

  // Should we let the editor deselect this as the current entity?
  // Probably not if we've made any changes.
  bool CanDeselectEntity() const { return !EntityModified(); }

  // Has the entity's flatbuffer been modified?
  bool EntityModified() const {
    for (auto iter = component_guis_.begin(); iter != component_guis_.end();
         ++iter) {
      FlatbufferEditor* editor = iter->second.get();
      if (editor->flatbuffer_modified()) return true;
    }
    return false;
  }

  // Does the user want you to show the current entity's physics?
  bool show_physics() const { return show_physics_; }

 private:
  enum GuiButton {
    kNone = 0,
    kWindowMaximize,
    kWindowHide,
    kWindowRestore,
    kToggleDataTypes,
    kToggleExpandAll,
    kTogglePhysics,
    kEntityCommit,
    kEntityRevert
  };

  enum EditView {
    kNoEditView = -1,
    kEditEntity = 0,
    kEntityList,
    kEditPrototype,
    kPrototypeList,
    kSettings,
    kEditViewCount
  };
  enum WindowState { kNormal, kMaximized };

  static const int kVirtualResolution = 1000;
  static const int kToolbarHeight = 30;
  static const int kSpacing = 3;
  static const int kBlankEditWidth = 20;
  static const int kIndent = 3;
  static const int kFontSize = 18;

  // Commit only the requested component flatbuffer to the entity.
  void CommitComponentData(entity::ComponentId id);

  // Send an EntityUpdated event to the current entity.
  void SendUpdateEvent();

  // Gui helper function to capture mouse clicks and prevent the world editor
  // from consuming them.
  void CaptureMouseClicks();

  // Draw an interface for editing an entity.
  void DrawEditEntityUI();
  // Draw an interface for choosing an entity.
  void DrawEntityListUI();
  // Draw an interface for changing editor settings.
  void DrawSettingsUI();
  // Draw a list of all of the component data that this entity has.
  void DrawEntityComponent(entity::ComponentId id);
  // Draw a list of the entity's parent and children, if any.
  void DrawEntityFamily();

  // Draw tab bars for the different views.
  void DrawTabs();

  void BeginDrawEditView();
  void FinishDrawEditView();

  // Create a text button; call this inside a gui::Run.
  gui::Event TextButton(const char* text, const char* id, int size);

  // Show a button that, if you click on it, selects another entity.
  void EntityButton(const entity::EntityRef& entity, int size);

  // Get the virtual resolution (for FlatUI) of the whole screen.
  void GetVirtualResolution(vec2* resolution_output);

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
  std::unordered_map<entity::ComponentId, std::unique_ptr<FlatbufferEditor>>
      component_guis_;
  entity::ComponentId auto_commit_component_;
  entity::ComponentId auto_revert_component_;
  entity::ComponentId auto_recreate_component_;

  std::vector<bool> components_to_show_;  // Components to display on screen.

  std::string entity_list_filter_;

  mathfu::vec4 bg_edit_ui_color_;
  mathfu::vec4 bg_toolbar_color_;
  mathfu::vec4 bg_button_color_;
  mathfu::vec4 bg_hover_color_;
  mathfu::vec4 bg_click_color_;

  mathfu::vec4 text_button_color_;
  mathfu::vec4 text_normal_color_;
  mathfu::vec4 text_disabled_color_;
  mathfu::vec4 text_editable_color_;
  mathfu::vec4 text_modified_color_;
  mathfu::vec4 text_error_color_;

  mathfu::vec2i scroll_offset_[kEditViewCount];
  mathfu::vec2 virtual_resolution_;
  GuiButton button_pressed_;
  WindowState edit_window_state_;
  EditView edit_view_;
  float edit_width_;
  bool show_physics_;
  bool show_types_;
  bool expand_all_;
  bool mouse_in_window_;
  bool keyboard_in_use_;
};

}  // namespace editor
}  // namespace fpl

#endif  // FPL_EDITOR_GUI_H_
