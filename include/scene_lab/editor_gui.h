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

#ifndef FPL_SCENE_LAB_EDITOR_GUI_H_
#define FPL_SCENE_LAB_EDITOR_GUI_H_

#include <memory>
#include <set>
#include <string>
#include <string.h>
#include <unordered_map>
#include "component_library/entity_factory.h"
#include "entity/entity_manager.h"
#include "flatui/flatui.h"
#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "scene_lab_config_generated.h"
#include "scene_lab/flatbuffer_editor.h"

namespace fpl {
namespace scene_lab {

class SceneLab;

/// EditorGui is a FlatUI-based GUI overlay for Scene Lab to the edit
/// window and on-screen menu buttons. Scene Lab can work without the
/// GUI but will have very reduced functionality.
class EditorGui {
 public:
  /// Initialize the user interface. Ensure that you pass the Flatbuffers schema
  /// you are using (in binary schema format) so FlatbufferEditor knows how to
  /// present the data. EditorGui will load its own font (specified in the
  /// config) into an existing FontManager.
  EditorGui(const SceneLabConfig* config, SceneLab* scene_lab,
            entity::EntityManager* entity_manager, FontManager* font_manager,
            const std::string* schema_data);

  /// Turn on the GUI. This should be called when Scene Lab is activated.
  void Activate();

  /// Turn off the GUI. This should be called when Scene Lab is deactivated.
  void Deactivate() {}

  /// Render the GUI, then update based on button presses. You must either call
  /// this, or call StartRender(), then DrawGui() in a FlatUI gui::Run context,
  /// then FinishRenderRender(). (only DrawGui() should be inside a FlatUI
  /// gui::Run context.)
  void Render();

  /// Prepare to draw the GUI. You don't need to call this if you are using
  /// Render(). See DrawGui() for more information.

  void StartRender();
  /// Either call Render() above, or call the following sequence if you are
  /// using FlatUI elsewhere:
  /// * StartRender();
  /// * gui::Run(() { ... other Gui... DrawGui(...) };
  /// * FinishRender();
  void DrawGui(const mathfu::vec2& virtual_resolution);

  /// Finish drawing the GUI, and update input. You don't need to call this if
  /// you are using Render(). See DrawGui() for more information.
  void FinishRender();

  /// Is the GUI capturing mouse and/or keyboard input? Returns true if you
  /// should ignore mouse and keyboard input. Or use mouse_in_window() and
  /// keyboard_in_use() for more fine-grained control.
  bool InputCaptured() const { return mouse_in_window() || keyboard_in_use(); }

  /// Is the mouse pointer over a GUI element? If true, you should ignore mouse
  /// clicks as the user is clicking on GUI elements instead.
  bool mouse_in_window() const { return mouse_in_window_; }

  /// If we've taken ownership of the keyboard (generally when a editable field
  /// is being edited), this will return true, telling you to ignore keyboard
  /// input.
  bool keyboard_in_use() const { return keyboard_in_use_; }

  /// Choose which entity we are currently editing.
  void SetEditEntity(entity::EntityRef& entity);
  /// Get the entity we are currently editing.
  entity::EntityRef edit_entity() const { return edit_entity_; }

  /// Clear all cached or modified data that we have for the edit entity. Call
  /// this if you change any entity data externally, so we can reload data
  /// directly from the entity.
  void ClearEntityData() { component_guis_.clear(); }
  /// Write the entity fields that were changed to the entity_data_ flatbuffer,
  /// then import the changed flatbuffers back to the edit_entity_.
  void CommitEntityData();

  /// Should we let the editor exit? Not if there are pending changes.
  bool CanExit();

  /// Should we let the editor deselect this as the current entity?
  /// Probably not if we've made any changes.
  bool CanDeselectEntity() const { return !EntityModified(); }

  /// Has the entity's flatbuffer been modified?
  bool EntityModified() const {
    for (auto iter = component_guis_.begin(); iter != component_guis_.end();
         ++iter) {
      FlatbufferEditor* editor = iter->second.get();
      if (editor->flatbuffer_modified()) return true;
    }
    return false;
  }

  /// "Entity Updated" callback for Scene Lab; if the entity is updated
  /// externally, we reload its data by calling ClearEntityData().
  void EntityUpdated(entity::EntityRef entity);

  /// Does the user want you to show the current entity's physics?
  bool show_physics() const { return show_physics_; }

  /// Returns which mouse mode index we have selected.
  unsigned int mouse_mode_index() const { return mouse_mode_index_; }
  /// Set which mouse mode index the user wants to use.
  void set_mouse_mode_index(int m) { mouse_mode_index_ = m; }

 private:
  /// Onscreen buttons IDs.
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

  /// Onscreen tabs for the edit window.
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
  static const int kButtonMargin = 5;

  /// Commit only the requested component flatbuffer to the entity.
  void CommitComponentData(entity::ComponentId id);

  /// Send an EntityUpdated event to the current entity.
  void SendUpdateEvent();

  /// Gui helper function to capture mouse clicks and prevent Scene Lab
  /// from consuming them.
  void CaptureMouseClicks();

  /// Draw an interface for editing an entity.
  void DrawEditEntityUI();
  /// Draw an interface for choosing an entity.
  void DrawEntityListUI();
  /// Draw an interface for changing editor settings.
  void DrawSettingsUI();
  /// Draw a list of all of the component data that this entity has.
  void DrawEntityComponent(entity::ComponentId id);
  /// Draw a list of the entity's parent and children, if any.
  void DrawEntityFamily();

  /// Draw tab bars for the different edit views.
  void DrawTabs();

  /// Draw common stuff at the beginning of an edit view.
  void BeginDrawEditView();
  /// Draw common stuff at the end of an edit view.
  void FinishDrawEditView();

  /// Create a text button; call this inside a gui::Run.
  gui::Event TextButton(const char* text, const char* id, int size);

  /// Show a button that, if you click on it, selects an entity.
  void EntityButton(const entity::EntityRef& entity, int size);

  /// Get the virtual resolution (for FlatUI) of the whole screen.
  void GetVirtualResolution(vec2* resolution_output);

  const SceneLabConfig* config_;
  SceneLab* scene_lab_;
  entity::EntityManager* entity_manager_;
  FontManager* font_manager_;
  const std::string* schema_data_;

  AssetManager* asset_manager_;
  component_library::EntityFactory* entity_factory_;
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

  mathfu::vec2 scroll_offset_[kEditViewCount];
  mathfu::vec2 virtual_resolution_;
  GuiButton button_pressed_;
  WindowState edit_window_state_;
  EditView edit_view_;
  float edit_width_;
  int mouse_mode_index_;
  bool show_physics_;        // Are we showing the selected entity's physics?
  bool show_types_;          // Are we showing the type of each field?
  bool expand_all_;          // Expand edit view to encompass the whole screen?
  bool mouse_in_window_;     // Is the mouse currently over a UI element?
  bool keyboard_in_use_;     // Is the user currently typing into an edit field?
  bool prompting_for_exit_;  // Are we currently prompting the user to exit?
  bool updated_via_gui_;     // Was the entity just updated via the GUI?
};

}  // namespace scene_lab
}  // namespace fpl

#endif  // FPL_SCENE_LAB_EDITOR_GUI_H_
