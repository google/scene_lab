// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_SCENELABCONFIG_SCENE_LAB_H_
#define FLATBUFFERS_GENERATED_SCENELABCONFIG_SCENE_LAB_H_

#include "flatbuffers/flatbuffers.h"

#include "flatbuffer_editor_config_generated.h"
#include "common_generated.h"

namespace scene_lab {

struct SceneLabConfig;

struct SceneLabConfig FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_SCHEMA_FILE_TEXT = 4,
    VT_SCHEMA_FILE_BINARY = 6,
    VT_SCHEMA_INCLUDE_PATHS = 8,
    VT_VIEWPORT_ANGLE_DEGREES = 10,
    VT_CAMERA_MOVEMENT_SPEED = 12,
    VT_CAMERA_MOVEMENT_PARALLEL_TO_GROUND = 14,
    VT_OBJECT_MOVEMENT_SPEED = 16,
    VT_OBJECT_ANGULAR_SPEED = 18,
    VT_OBJECT_SCALE_SPEED = 20,
    VT_PRECISE_MOVEMENT_SCALE = 22,
    VT_ENTITY_SPAWN_DISTANCE = 24,
    VT_MOUSE_SENSITIVITY = 26,
    VT_INTERACT_BUTTON = 28,
    VT_TOGGLE_MODE_BUTTON = 30,
    VT_GUI_ALLOW_EDITING = 32,
    VT_GUI_PROMPT_FOR_SAVE_ON_EXIT = 34,
    VT_GUI_BUTTON_SIZE = 36,
    VT_GUI_TOOLBAR_SIZE = 38,
    VT_GUI_BG_TOOLBAR_COLOR = 40,
    VT_GUI_BG_EDIT_UI_COLOR = 42,
    VT_FLATBUFFER_EDITOR_CONFIG = 44,
    VT_BINARY_ENTITY_FILE_EXT = 46,
    VT_GUI_FONT = 48,
    VT_JSON_OUTPUT_DIRECTORY = 50
  };
  const flatbuffers::String *schema_file_text() const { return GetPointer<const flatbuffers::String *>(VT_SCHEMA_FILE_TEXT); }
  flatbuffers::String *mutable_schema_file_text() { return GetPointer<flatbuffers::String *>(VT_SCHEMA_FILE_TEXT); }
  const flatbuffers::String *schema_file_binary() const { return GetPointer<const flatbuffers::String *>(VT_SCHEMA_FILE_BINARY); }
  flatbuffers::String *mutable_schema_file_binary() { return GetPointer<flatbuffers::String *>(VT_SCHEMA_FILE_BINARY); }
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *schema_include_paths() const { return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_SCHEMA_INCLUDE_PATHS); }
  flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *mutable_schema_include_paths() { return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_SCHEMA_INCLUDE_PATHS); }
  float viewport_angle_degrees() const { return GetField<float>(VT_VIEWPORT_ANGLE_DEGREES, 0.0f); }
  bool mutate_viewport_angle_degrees(float _viewport_angle_degrees) { return SetField(VT_VIEWPORT_ANGLE_DEGREES, _viewport_angle_degrees); }
  float camera_movement_speed() const { return GetField<float>(VT_CAMERA_MOVEMENT_SPEED, 0.0f); }
  bool mutate_camera_movement_speed(float _camera_movement_speed) { return SetField(VT_CAMERA_MOVEMENT_SPEED, _camera_movement_speed); }
  bool camera_movement_parallel_to_ground() const { return GetField<uint8_t>(VT_CAMERA_MOVEMENT_PARALLEL_TO_GROUND, 1) != 0; }
  bool mutate_camera_movement_parallel_to_ground(bool _camera_movement_parallel_to_ground) { return SetField(VT_CAMERA_MOVEMENT_PARALLEL_TO_GROUND, static_cast<uint8_t>(_camera_movement_parallel_to_ground)); }
  float object_movement_speed() const { return GetField<float>(VT_OBJECT_MOVEMENT_SPEED, 0.0f); }
  bool mutate_object_movement_speed(float _object_movement_speed) { return SetField(VT_OBJECT_MOVEMENT_SPEED, _object_movement_speed); }
  float object_angular_speed() const { return GetField<float>(VT_OBJECT_ANGULAR_SPEED, 0.0f); }
  bool mutate_object_angular_speed(float _object_angular_speed) { return SetField(VT_OBJECT_ANGULAR_SPEED, _object_angular_speed); }
  float object_scale_speed() const { return GetField<float>(VT_OBJECT_SCALE_SPEED, 0.0f); }
  bool mutate_object_scale_speed(float _object_scale_speed) { return SetField(VT_OBJECT_SCALE_SPEED, _object_scale_speed); }
  float precise_movement_scale() const { return GetField<float>(VT_PRECISE_MOVEMENT_SCALE, 0.0f); }
  bool mutate_precise_movement_scale(float _precise_movement_scale) { return SetField(VT_PRECISE_MOVEMENT_SCALE, _precise_movement_scale); }
  float entity_spawn_distance() const { return GetField<float>(VT_ENTITY_SPAWN_DISTANCE, 20.0f); }
  bool mutate_entity_spawn_distance(float _entity_spawn_distance) { return SetField(VT_ENTITY_SPAWN_DISTANCE, _entity_spawn_distance); }
  float mouse_sensitivity() const { return GetField<float>(VT_MOUSE_SENSITIVITY, 0.0f); }
  bool mutate_mouse_sensitivity(float _mouse_sensitivity) { return SetField(VT_MOUSE_SENSITIVITY, _mouse_sensitivity); }
  int8_t interact_button() const { return GetField<int8_t>(VT_INTERACT_BUTTON, 0); }
  bool mutate_interact_button(int8_t _interact_button) { return SetField(VT_INTERACT_BUTTON, _interact_button); }
  int8_t toggle_mode_button() const { return GetField<int8_t>(VT_TOGGLE_MODE_BUTTON, 2); }
  bool mutate_toggle_mode_button(int8_t _toggle_mode_button) { return SetField(VT_TOGGLE_MODE_BUTTON, _toggle_mode_button); }
  bool gui_allow_editing() const { return GetField<uint8_t>(VT_GUI_ALLOW_EDITING, 1) != 0; }
  bool mutate_gui_allow_editing(bool _gui_allow_editing) { return SetField(VT_GUI_ALLOW_EDITING, static_cast<uint8_t>(_gui_allow_editing)); }
  bool gui_prompt_for_save_on_exit() const { return GetField<uint8_t>(VT_GUI_PROMPT_FOR_SAVE_ON_EXIT, 1) != 0; }
  bool mutate_gui_prompt_for_save_on_exit(bool _gui_prompt_for_save_on_exit) { return SetField(VT_GUI_PROMPT_FOR_SAVE_ON_EXIT, static_cast<uint8_t>(_gui_prompt_for_save_on_exit)); }
  float gui_button_size() const { return GetField<float>(VT_GUI_BUTTON_SIZE, 22.0f); }
  bool mutate_gui_button_size(float _gui_button_size) { return SetField(VT_GUI_BUTTON_SIZE, _gui_button_size); }
  float gui_toolbar_size() const { return GetField<float>(VT_GUI_TOOLBAR_SIZE, 26.0f); }
  bool mutate_gui_toolbar_size(float _gui_toolbar_size) { return SetField(VT_GUI_TOOLBAR_SIZE, _gui_toolbar_size); }
  const fplbase::ColorRGBA *gui_bg_toolbar_color() const { return GetStruct<const fplbase::ColorRGBA *>(VT_GUI_BG_TOOLBAR_COLOR); }
  fplbase::ColorRGBA *mutable_gui_bg_toolbar_color() { return GetStruct<fplbase::ColorRGBA *>(VT_GUI_BG_TOOLBAR_COLOR); }
  const fplbase::ColorRGBA *gui_bg_edit_ui_color() const { return GetStruct<const fplbase::ColorRGBA *>(VT_GUI_BG_EDIT_UI_COLOR); }
  fplbase::ColorRGBA *mutable_gui_bg_edit_ui_color() { return GetStruct<fplbase::ColorRGBA *>(VT_GUI_BG_EDIT_UI_COLOR); }
  const scene_lab::FlatbufferEditorConfig *flatbuffer_editor_config() const { return GetPointer<const scene_lab::FlatbufferEditorConfig *>(VT_FLATBUFFER_EDITOR_CONFIG); }
  scene_lab::FlatbufferEditorConfig *mutable_flatbuffer_editor_config() { return GetPointer<scene_lab::FlatbufferEditorConfig *>(VT_FLATBUFFER_EDITOR_CONFIG); }
  const flatbuffers::String *binary_entity_file_ext() const { return GetPointer<const flatbuffers::String *>(VT_BINARY_ENTITY_FILE_EXT); }
  flatbuffers::String *mutable_binary_entity_file_ext() { return GetPointer<flatbuffers::String *>(VT_BINARY_ENTITY_FILE_EXT); }
  const flatbuffers::String *gui_font() const { return GetPointer<const flatbuffers::String *>(VT_GUI_FONT); }
  flatbuffers::String *mutable_gui_font() { return GetPointer<flatbuffers::String *>(VT_GUI_FONT); }
  const flatbuffers::String *json_output_directory() const { return GetPointer<const flatbuffers::String *>(VT_JSON_OUTPUT_DIRECTORY); }
  flatbuffers::String *mutable_json_output_directory() { return GetPointer<flatbuffers::String *>(VT_JSON_OUTPUT_DIRECTORY); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_SCHEMA_FILE_TEXT) &&
           verifier.Verify(schema_file_text()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_SCHEMA_FILE_BINARY) &&
           verifier.Verify(schema_file_binary()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_SCHEMA_INCLUDE_PATHS) &&
           verifier.Verify(schema_include_paths()) &&
           verifier.VerifyVectorOfStrings(schema_include_paths()) &&
           VerifyField<float>(verifier, VT_VIEWPORT_ANGLE_DEGREES) &&
           VerifyField<float>(verifier, VT_CAMERA_MOVEMENT_SPEED) &&
           VerifyField<uint8_t>(verifier, VT_CAMERA_MOVEMENT_PARALLEL_TO_GROUND) &&
           VerifyField<float>(verifier, VT_OBJECT_MOVEMENT_SPEED) &&
           VerifyField<float>(verifier, VT_OBJECT_ANGULAR_SPEED) &&
           VerifyField<float>(verifier, VT_OBJECT_SCALE_SPEED) &&
           VerifyField<float>(verifier, VT_PRECISE_MOVEMENT_SCALE) &&
           VerifyField<float>(verifier, VT_ENTITY_SPAWN_DISTANCE) &&
           VerifyField<float>(verifier, VT_MOUSE_SENSITIVITY) &&
           VerifyField<int8_t>(verifier, VT_INTERACT_BUTTON) &&
           VerifyField<int8_t>(verifier, VT_TOGGLE_MODE_BUTTON) &&
           VerifyField<uint8_t>(verifier, VT_GUI_ALLOW_EDITING) &&
           VerifyField<uint8_t>(verifier, VT_GUI_PROMPT_FOR_SAVE_ON_EXIT) &&
           VerifyField<float>(verifier, VT_GUI_BUTTON_SIZE) &&
           VerifyField<float>(verifier, VT_GUI_TOOLBAR_SIZE) &&
           VerifyField<fplbase::ColorRGBA>(verifier, VT_GUI_BG_TOOLBAR_COLOR) &&
           VerifyField<fplbase::ColorRGBA>(verifier, VT_GUI_BG_EDIT_UI_COLOR) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_FLATBUFFER_EDITOR_CONFIG) &&
           verifier.VerifyTable(flatbuffer_editor_config()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_BINARY_ENTITY_FILE_EXT) &&
           verifier.Verify(binary_entity_file_ext()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_GUI_FONT) &&
           verifier.Verify(gui_font()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, VT_JSON_OUTPUT_DIRECTORY) &&
           verifier.Verify(json_output_directory()) &&
           verifier.EndTable();
  }
};

struct SceneLabConfigBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_schema_file_text(flatbuffers::Offset<flatbuffers::String> schema_file_text) { fbb_.AddOffset(SceneLabConfig::VT_SCHEMA_FILE_TEXT, schema_file_text); }
  void add_schema_file_binary(flatbuffers::Offset<flatbuffers::String> schema_file_binary) { fbb_.AddOffset(SceneLabConfig::VT_SCHEMA_FILE_BINARY, schema_file_binary); }
  void add_schema_include_paths(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> schema_include_paths) { fbb_.AddOffset(SceneLabConfig::VT_SCHEMA_INCLUDE_PATHS, schema_include_paths); }
  void add_viewport_angle_degrees(float viewport_angle_degrees) { fbb_.AddElement<float>(SceneLabConfig::VT_VIEWPORT_ANGLE_DEGREES, viewport_angle_degrees, 0.0f); }
  void add_camera_movement_speed(float camera_movement_speed) { fbb_.AddElement<float>(SceneLabConfig::VT_CAMERA_MOVEMENT_SPEED, camera_movement_speed, 0.0f); }
  void add_camera_movement_parallel_to_ground(bool camera_movement_parallel_to_ground) { fbb_.AddElement<uint8_t>(SceneLabConfig::VT_CAMERA_MOVEMENT_PARALLEL_TO_GROUND, static_cast<uint8_t>(camera_movement_parallel_to_ground), 1); }
  void add_object_movement_speed(float object_movement_speed) { fbb_.AddElement<float>(SceneLabConfig::VT_OBJECT_MOVEMENT_SPEED, object_movement_speed, 0.0f); }
  void add_object_angular_speed(float object_angular_speed) { fbb_.AddElement<float>(SceneLabConfig::VT_OBJECT_ANGULAR_SPEED, object_angular_speed, 0.0f); }
  void add_object_scale_speed(float object_scale_speed) { fbb_.AddElement<float>(SceneLabConfig::VT_OBJECT_SCALE_SPEED, object_scale_speed, 0.0f); }
  void add_precise_movement_scale(float precise_movement_scale) { fbb_.AddElement<float>(SceneLabConfig::VT_PRECISE_MOVEMENT_SCALE, precise_movement_scale, 0.0f); }
  void add_entity_spawn_distance(float entity_spawn_distance) { fbb_.AddElement<float>(SceneLabConfig::VT_ENTITY_SPAWN_DISTANCE, entity_spawn_distance, 20.0f); }
  void add_mouse_sensitivity(float mouse_sensitivity) { fbb_.AddElement<float>(SceneLabConfig::VT_MOUSE_SENSITIVITY, mouse_sensitivity, 0.0f); }
  void add_interact_button(int8_t interact_button) { fbb_.AddElement<int8_t>(SceneLabConfig::VT_INTERACT_BUTTON, interact_button, 0); }
  void add_toggle_mode_button(int8_t toggle_mode_button) { fbb_.AddElement<int8_t>(SceneLabConfig::VT_TOGGLE_MODE_BUTTON, toggle_mode_button, 2); }
  void add_gui_allow_editing(bool gui_allow_editing) { fbb_.AddElement<uint8_t>(SceneLabConfig::VT_GUI_ALLOW_EDITING, static_cast<uint8_t>(gui_allow_editing), 1); }
  void add_gui_prompt_for_save_on_exit(bool gui_prompt_for_save_on_exit) { fbb_.AddElement<uint8_t>(SceneLabConfig::VT_GUI_PROMPT_FOR_SAVE_ON_EXIT, static_cast<uint8_t>(gui_prompt_for_save_on_exit), 1); }
  void add_gui_button_size(float gui_button_size) { fbb_.AddElement<float>(SceneLabConfig::VT_GUI_BUTTON_SIZE, gui_button_size, 22.0f); }
  void add_gui_toolbar_size(float gui_toolbar_size) { fbb_.AddElement<float>(SceneLabConfig::VT_GUI_TOOLBAR_SIZE, gui_toolbar_size, 26.0f); }
  void add_gui_bg_toolbar_color(const fplbase::ColorRGBA *gui_bg_toolbar_color) { fbb_.AddStruct(SceneLabConfig::VT_GUI_BG_TOOLBAR_COLOR, gui_bg_toolbar_color); }
  void add_gui_bg_edit_ui_color(const fplbase::ColorRGBA *gui_bg_edit_ui_color) { fbb_.AddStruct(SceneLabConfig::VT_GUI_BG_EDIT_UI_COLOR, gui_bg_edit_ui_color); }
  void add_flatbuffer_editor_config(flatbuffers::Offset<scene_lab::FlatbufferEditorConfig> flatbuffer_editor_config) { fbb_.AddOffset(SceneLabConfig::VT_FLATBUFFER_EDITOR_CONFIG, flatbuffer_editor_config); }
  void add_binary_entity_file_ext(flatbuffers::Offset<flatbuffers::String> binary_entity_file_ext) { fbb_.AddOffset(SceneLabConfig::VT_BINARY_ENTITY_FILE_EXT, binary_entity_file_ext); }
  void add_gui_font(flatbuffers::Offset<flatbuffers::String> gui_font) { fbb_.AddOffset(SceneLabConfig::VT_GUI_FONT, gui_font); }
  void add_json_output_directory(flatbuffers::Offset<flatbuffers::String> json_output_directory) { fbb_.AddOffset(SceneLabConfig::VT_JSON_OUTPUT_DIRECTORY, json_output_directory); }
  SceneLabConfigBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  SceneLabConfigBuilder &operator=(const SceneLabConfigBuilder &);
  flatbuffers::Offset<SceneLabConfig> Finish() {
    auto o = flatbuffers::Offset<SceneLabConfig>(fbb_.EndTable(start_, 24));
    return o;
  }
};

inline flatbuffers::Offset<SceneLabConfig> CreateSceneLabConfig(flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> schema_file_text = 0,
    flatbuffers::Offset<flatbuffers::String> schema_file_binary = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> schema_include_paths = 0,
    float viewport_angle_degrees = 0.0f,
    float camera_movement_speed = 0.0f,
    bool camera_movement_parallel_to_ground = true,
    float object_movement_speed = 0.0f,
    float object_angular_speed = 0.0f,
    float object_scale_speed = 0.0f,
    float precise_movement_scale = 0.0f,
    float entity_spawn_distance = 20.0f,
    float mouse_sensitivity = 0.0f,
    int8_t interact_button = 0,
    int8_t toggle_mode_button = 2,
    bool gui_allow_editing = true,
    bool gui_prompt_for_save_on_exit = true,
    float gui_button_size = 22.0f,
    float gui_toolbar_size = 26.0f,
    const fplbase::ColorRGBA *gui_bg_toolbar_color = 0,
    const fplbase::ColorRGBA *gui_bg_edit_ui_color = 0,
    flatbuffers::Offset<scene_lab::FlatbufferEditorConfig> flatbuffer_editor_config = 0,
    flatbuffers::Offset<flatbuffers::String> binary_entity_file_ext = 0,
    flatbuffers::Offset<flatbuffers::String> gui_font = 0,
    flatbuffers::Offset<flatbuffers::String> json_output_directory = 0) {
  SceneLabConfigBuilder builder_(_fbb);
  builder_.add_json_output_directory(json_output_directory);
  builder_.add_gui_font(gui_font);
  builder_.add_binary_entity_file_ext(binary_entity_file_ext);
  builder_.add_flatbuffer_editor_config(flatbuffer_editor_config);
  builder_.add_gui_bg_edit_ui_color(gui_bg_edit_ui_color);
  builder_.add_gui_bg_toolbar_color(gui_bg_toolbar_color);
  builder_.add_gui_toolbar_size(gui_toolbar_size);
  builder_.add_gui_button_size(gui_button_size);
  builder_.add_mouse_sensitivity(mouse_sensitivity);
  builder_.add_entity_spawn_distance(entity_spawn_distance);
  builder_.add_precise_movement_scale(precise_movement_scale);
  builder_.add_object_scale_speed(object_scale_speed);
  builder_.add_object_angular_speed(object_angular_speed);
  builder_.add_object_movement_speed(object_movement_speed);
  builder_.add_camera_movement_speed(camera_movement_speed);
  builder_.add_viewport_angle_degrees(viewport_angle_degrees);
  builder_.add_schema_include_paths(schema_include_paths);
  builder_.add_schema_file_binary(schema_file_binary);
  builder_.add_schema_file_text(schema_file_text);
  builder_.add_gui_prompt_for_save_on_exit(gui_prompt_for_save_on_exit);
  builder_.add_gui_allow_editing(gui_allow_editing);
  builder_.add_toggle_mode_button(toggle_mode_button);
  builder_.add_interact_button(interact_button);
  builder_.add_camera_movement_parallel_to_ground(camera_movement_parallel_to_ground);
  return builder_.Finish();
}

inline flatbuffers::Offset<SceneLabConfig> CreateSceneLabConfigDirect(flatbuffers::FlatBufferBuilder &_fbb,
    const char *schema_file_text = nullptr,
    const char *schema_file_binary = nullptr,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *schema_include_paths = nullptr,
    float viewport_angle_degrees = 0.0f,
    float camera_movement_speed = 0.0f,
    bool camera_movement_parallel_to_ground = true,
    float object_movement_speed = 0.0f,
    float object_angular_speed = 0.0f,
    float object_scale_speed = 0.0f,
    float precise_movement_scale = 0.0f,
    float entity_spawn_distance = 20.0f,
    float mouse_sensitivity = 0.0f,
    int8_t interact_button = 0,
    int8_t toggle_mode_button = 2,
    bool gui_allow_editing = true,
    bool gui_prompt_for_save_on_exit = true,
    float gui_button_size = 22.0f,
    float gui_toolbar_size = 26.0f,
    const fplbase::ColorRGBA *gui_bg_toolbar_color = 0,
    const fplbase::ColorRGBA *gui_bg_edit_ui_color = 0,
    flatbuffers::Offset<scene_lab::FlatbufferEditorConfig> flatbuffer_editor_config = 0,
    const char *binary_entity_file_ext = nullptr,
    const char *gui_font = nullptr,
    const char *json_output_directory = nullptr) {
  return CreateSceneLabConfig(_fbb, schema_file_text ? _fbb.CreateString(schema_file_text) : 0, schema_file_binary ? _fbb.CreateString(schema_file_binary) : 0, schema_include_paths ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*schema_include_paths) : 0, viewport_angle_degrees, camera_movement_speed, camera_movement_parallel_to_ground, object_movement_speed, object_angular_speed, object_scale_speed, precise_movement_scale, entity_spawn_distance, mouse_sensitivity, interact_button, toggle_mode_button, gui_allow_editing, gui_prompt_for_save_on_exit, gui_button_size, gui_toolbar_size, gui_bg_toolbar_color, gui_bg_edit_ui_color, flatbuffer_editor_config, binary_entity_file_ext ? _fbb.CreateString(binary_entity_file_ext) : 0, gui_font ? _fbb.CreateString(gui_font) : 0, json_output_directory ? _fbb.CreateString(json_output_directory) : 0);
}

inline const scene_lab::SceneLabConfig *GetSceneLabConfig(const void *buf) {
  return flatbuffers::GetRoot<scene_lab::SceneLabConfig>(buf);
}

inline SceneLabConfig *GetMutableSceneLabConfig(void *buf) {
  return flatbuffers::GetMutableRoot<SceneLabConfig>(buf);
}

inline bool VerifySceneLabConfigBuffer(flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<scene_lab::SceneLabConfig>(nullptr);
}

inline void FinishSceneLabConfigBuffer(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<scene_lab::SceneLabConfig> root) {
  fbb.Finish(root);
}

}  // namespace scene_lab

#endif  // FLATBUFFERS_GENERATED_SCENELABCONFIG_SCENE_LAB_H_
