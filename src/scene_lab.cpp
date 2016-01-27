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

#include "scene_lab/scene_lab.h"

#include <math.h>
#include <set>
#include <string>
#include <unordered_map>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/utilities.h"
#include "scene_lab/basic_camera.h"

namespace scene_lab {

using fplbase::Vec3;
using mathfu::mat4;
using mathfu::vec3;
using mathfu::vec4;

static const float kMinValidDistance = 0.00001f;

static const char kDefaultBinaryEntityFileExtension[] = "bin";

static const char kDefaultEntityFile[] = "entities_default";

// String which identifies the current version of Scene Lab. See the comment on
// kVersion in scene_lab.h for more information on how this is used.
const char SceneLab::kVersion[] = "Scene Lab 1.1.0";

void SceneLab::Initialize(const SceneLabConfig* config,
                          fplbase::AssetManager* asset_manager,
                          fplbase::InputSystem* input,
                          fplbase::Renderer* renderer,
                          flatui::FontManager* font_manager) {
  version_ = kVersion;
  config_ = config;
  font_manager_ = font_manager;

  asset_manager_ = asset_manager;
  renderer_ = renderer;
  input_system_ = input;

  if (config_->gui_font() != nullptr) {
    font_manager_->Open(config_->gui_font()->c_str());
  }

  horizontal_forward_ = mathfu::kAxisY3f;
  horizontal_right_ = mathfu::kAxisX3f;
  controller_.reset(new EditorController(config_, input_system_));
  input_mode_ = kMoving;
  mouse_mode_ = kMoveHorizontal;
  gui_.reset(new EditorGui(config_, this, asset_manager_, input_system_,
                           renderer_, font_manager_));
  initial_camera_set_ = false;
}

void SceneLab::SetEntitySystemAdapter(
    std::unique_ptr<EntitySystemAdapter> adapter) {
  entity_system_adapter_ = std::move(adapter);
}

// Project `v` onto `unit`. That is, return the vector colinear with `unit`
// such that `v` - returned_vector is perpendicular to `unit`.
static inline vec3 ProjectOntoUnitVector(const vec3& v, const vec3& unit) {
  return vec3::DotProduct(v, unit) * unit;
}

void SceneLab::AdvanceFrame(double time_delta_seconds) {
  GenericCamera camera;
  entity_system_adapter()->GetCamera(&camera);

  // Update the editor's forward and right vectors in the horizontal plane.
  // Remove the up component from the camera's facing vector.
  vec3 forward =
      camera.facing - ProjectOntoUnitVector(camera.facing, camera.up);
  vec3 right = vec3::CrossProduct(camera.facing, camera.up);

  // If we're in gimbal lock, use previous frame's forward calculations.
  if (forward.Normalize() > kMinValidDistance &&
      right.Normalize() > kMinValidDistance) {
    horizontal_forward_ = forward;
    horizontal_right_ = right;
  }

  if (input_mode_ == kMoving) {
    // Allow the camera to look around and move.
    camera.facing = controller_->GetFacing();

    vec3 movement = GetMovement();
    camera.position =
        camera.position + movement * static_cast<float>(time_delta_seconds);

    entity_system_adapter()->SetCamera(camera);

    if (!gui_->InputCaptured() &&
        controller_->ButtonWentDown(config_->toggle_mode_button())) {
      input_mode_ = kEditing;
      controller_->UnlockMouse();
    }
  } else if (input_mode_ == kEditing) {
    if (!gui_->InputCaptured() &&
        controller_->ButtonWentDown(config_->toggle_mode_button())) {
      controller_->SetFacing(camera.facing);
      controller_->LockMouse();
      input_mode_ = kMoving;
    }
  } else if (input_mode_ == kDragging) {
    if (controller_->ButtonWentUp(config_->interact_button())) {
      input_mode_ = kEditing;
    }

    if (!gui_->InputCaptured() &&
        controller_->ButtonWentDown(config_->toggle_mode_button())) {
      controller_->SetFacing(camera.facing);
      controller_->LockMouse();
      input_mode_ = kMoving;
    }
  }

  GenericEntityId next_entity = EntitySystemAdapter::kNoEntityId;
  if (gui_->CanDeselectEntity()) {
    if (!gui_->InputCaptured() &&
        controller_->KeyWentDown(fplbase::FPLK_RIGHTBRACKET)) {
      entity_system_adapter()->CycleEntities(1, &next_entity);
    }
    if (!gui_->InputCaptured() &&
        controller_->KeyWentDown(fplbase::FPLK_LEFTBRACKET)) {
      entity_system_adapter()->CycleEntities(-1, &next_entity);
    }
  }
  if (next_entity != EntitySystemAdapter::kNoEntityId &&
      next_entity != selected_entity_) {
    SelectEntity(next_entity);
  }

  GenericEntityId clicked_entity = EntitySystemAdapter::kNoEntityId;

  if (!gui_->InputCaptured() && gui_->CanDeselectEntity() &&
      controller_->ButtonWentDown(config_->interact_button())) {
    // Use position of the mouse pointer for the ray cast.
    vec3 start, dir;
    bool got_ray = false;
    if (controller_->mouse_locked()) {
      start = camera.position;
      dir = camera.facing;
      got_ray = true;
    } else {
      ViewportSettings viewport;
      if (entity_system_adapter()->GetViewportSettings(&viewport)) {
        mathfu::vec2 pointer = controller_->GetPointer();
        if (controller_->ScreenPointToWorldRay(camera, viewport, pointer,
                                               renderer_->window_size(), &start,
                                               &dir)) {
          got_ray = true;
        }
      }
    }
    if (got_ray) {
      if (!entity_system_adapter()->GetRayIntersection(
              start, dir, &clicked_entity, &drag_point_)) {
        // Clicked on no entity.
        SelectEntity(EntitySystemAdapter::kNoEntityId);
      }
      // If we clicked an entity, clicked_entity will be set.
    }
  }
  bool start_dragging = false;
  if (clicked_entity != EntitySystemAdapter::kNoEntityId) {
    if (input_mode_ == kEditing && clicked_entity == selected_entity_) {
      // If we click on the same entity again, we start dragging it around.
      start_dragging = true;
    } else {
      // If we click on an entity for the first time, select it.
      SelectEntity(clicked_entity);
    }
  }

  if (selected_entity_ != EntitySystemAdapter::kNoEntityId) {
    // We have an entity selected, let's allow it to be modified
    GenericTransform transform;
    if (entity_system_adapter()->GetEntityTransform(selected_entity_,
                                                    &transform)) {
      if (ModifyTransformBasedOnInput(&transform)) {
        set_entities_modified(true);
        entity_system_adapter()->SetEntityTransform(selected_entity_,
                                                    transform);
        NotifyUpdateEntity(selected_entity_);
      }
    }

    if (!gui_->InputCaptured() &&
        (controller_->KeyWentDown(fplbase::FPLK_INSERT) ||
         controller_->KeyWentDown(fplbase::FPLK_v))) {
      GenericEntityId new_entity;
      if (entity_system_adapter()->DuplicateEntity(selected_entity_,
                                                   &new_entity)) {
        SelectEntity(new_entity);
        NotifyUpdateEntity(new_entity);
      }
    }
    if (!gui_->InputCaptured() &&
        (controller_->KeyWentDown(fplbase::FPLK_DELETE) ||
         controller_->KeyWentDown(fplbase::FPLK_x))) {
      NotifyDeleteEntity(selected_entity_);
      if (entity_system_adapter()->DeleteEntity(selected_entity_)) {
        SelectEntity(EntitySystemAdapter::kNoEntityId);
      }
    }
  }

  entity_system_adapter()->AdvanceFrame(time_delta_seconds);

  if (start_dragging && input_mode_ == kEditing &&
      selected_entity_ != EntitySystemAdapter::kNoEntityId) {
    GenericTransform transform;
    ViewportSettings viewport;
    if (entity_system_adapter()->GetEntityTransform(selected_entity_,
                                                    &transform) &&
        entity_system_adapter()->GetViewportSettings(&viewport)) {
      vec3 position = transform.position;
      vec3 intersect;

      mathfu::vec2 pointer = controller_->GetPointer();
      vec3 mouse_ray_origin;
      vec3 mouse_ray_dir;
      controller_->ScreenPointToWorldRay(camera, viewport, pointer,
                                         renderer_->window_size(),
                                         &mouse_ray_origin, &mouse_ray_dir);

      if (mouse_mode_ == kScaleX || mouse_mode_ == kScaleY ||
          mouse_mode_ == kScaleZ || mouse_mode_ == kScaleAll) {
        // For scaling, the normal for the drag plane is just a vector pointing
        // back towards the camera.
        drag_plane_normal_ = -camera.facing;
      } else if (mouse_mode_ == kMoveVertical ||
                 mouse_mode_ == kRotateVertical) {
        // For Vertical actions, the normal for the drag plane is a vector
        // pointing back towards the camera with the up component zeroed, so
        // it's
        // perpendicular to the ground.
        drag_plane_normal_ = -camera.facing;
        drag_plane_normal_.z() = 0;
        drag_plane_normal_.Normalize();
      } else {
        // For Horizontal, the normal for the drag plane is the up-vector.
        drag_plane_normal_ = vec3(0, 0, 1);
      }
      if (IntersectRayToPlane(mouse_ray_origin, mouse_ray_dir, drag_point_,
                              drag_plane_normal_, &intersect)) {
        drag_offset_ = position - intersect;
        drag_prev_intersect_ = intersect;
        drag_orig_scale_ = transform.scale;

        input_mode_ = kDragging;
      }
    }
  }

  if (input_mode_ != kDragging) {
    // If not dragging, then see if we changed the mouse mode.
    unsigned int mode_idx = gui_->mouse_mode_index();
    if (mode_idx < kMouseModeCount) {
      mouse_mode_ = static_cast<MouseMode>(mode_idx);
    }
  } else {
    // If we are still dragging, don't allow the mouse mode to be changed.
    gui_->set_mouse_mode_index(mouse_mode_);
  }

  if (exit_requested_ && gui_->CanExit()) {
    exit_ready_ = !entities_modified_;
  } else {
    exit_ready_ = false;
  }
}

void SceneLab::SelectEntity(const GenericEntityId& entity_id) {
  if (selected_entity_ != EntitySystemAdapter::kNoEntityId &&
      selected_entity_ != entity_id) {
    // Un-highlight the old entity.
    entity_system_adapter()->SetEntityHighlighted(selected_entity_, false);
  }
  if (entity_id == EntitySystemAdapter::kNoEntityId) {
    // Select no entity.
    selected_entity_ = entity_id;
  } else if (entity_system_adapter()->EntityExists(entity_id)) {
    // Select and highlight the new entity.
    selected_entity_ = entity_id;
    entity_system_adapter()->SetEntityHighlighted(selected_entity_, true);
  }
}

void SceneLab::MoveEntityToCamera(const GenericEntityId& id) {
  GenericCamera camera;
  entity_system_adapter()->GetCamera(&camera);
  GenericTransform transform;
  if (entity_system_adapter()->GetEntityTransform(id, &transform)) {
    transform.position =
        camera.position + camera.facing * config_->entity_spawn_distance();
    if (transform.position.z() < 0) transform.position.z() = 0;
    entity_system_adapter()->SetEntityTransform(id, transform);
  }
}

void SceneLab::Render(fplbase::Renderer* /*renderer*/) {
  // Render any editor-specific things
  gui_->SetEditEntity(selected_entity_);
  if (selected_entity_ != EntitySystemAdapter::kNoEntityId &&
      gui_->show_physics()) {
    entity_system_adapter()->DebugDrawPhysics(selected_entity_);
  }
  gui_->Render();
  if (gui_->edit_entity() != selected_entity_) {
    // The GUI changed who we have selected.
    SelectEntity(gui_->edit_entity());
  }
  controller_->Update();  // update the controller now so we can block its input
                          // via the gui
}

void SceneLab::SetInitialCamera(const GenericCamera& initial_camera) {
  // If we can't set the initial camera now, queue it for later.
  initial_camera_set_ = false;
  if (entity_system_adapter() == nullptr ||
      !entity_system_adapter()->SetCamera(initial_camera)) {
    initial_camera_set_ = true;
    initial_camera_ = initial_camera;
  }
}

void SceneLab::GetCamera(GenericCamera* camera) {
  entity_system_adapter()->GetCamera(camera);
}

void SceneLab::NotifyEnterEditor() const {
  for (auto iter = on_enter_editor_callbacks_.begin();
       iter != on_enter_editor_callbacks_.end(); ++iter) {
    (*iter)();
  }
}

void SceneLab::NotifyExitEditor() const {
  for (auto iter = on_exit_editor_callbacks_.begin();
       iter != on_exit_editor_callbacks_.end(); ++iter) {
    (*iter)();
  }
}

void SceneLab::NotifyCreateEntity(const GenericEntityId& entity) const {
  entity_system_adapter()->OnEntityCreated(entity);
  for (auto iter = on_create_entity_callbacks_.begin();
       iter != on_create_entity_callbacks_.end(); ++iter) {
    (*iter)(entity);
  }
}

void SceneLab::NotifyUpdateEntity(const GenericEntityId& entity) const {
  entity_system_adapter()->OnEntityUpdated(entity);
  for (auto iter = on_update_entity_callbacks_.begin();
       iter != on_update_entity_callbacks_.end(); ++iter) {
    (*iter)(entity);
  }
}

void SceneLab::NotifyDeleteEntity(const GenericEntityId& entity) const {
  entity_system_adapter()->OnEntityDeleted(entity);
  for (auto iter = on_delete_entity_callbacks_.begin();
       iter != on_delete_entity_callbacks_.end(); ++iter) {
    (*iter)(entity);
  }
}

void SceneLab::Activate() {
  exit_requested_ = false;
  exit_ready_ = false;
  set_entities_modified(false);

  input_mode_ = kMoving;

  NotifyEnterEditor();

  gui_->Activate();

  // De-select all entities.
  SelectEntity(EntitySystemAdapter::kNoEntityId);

  entity_system_adapter()->OnActivate();

  if (initial_camera_set_) {
    initial_camera_set_ = false;
    entity_system_adapter()->SetCamera(initial_camera_);
  }
  GenericCamera camera;
  entity_system_adapter()->GetCamera(&camera);
  // Set up the initial camera position.
  controller_->SetFacing(camera.facing);
  controller_->LockMouse();
}

void SceneLab::Deactivate() {
  // De-select all entities.
  SelectEntity(EntitySystemAdapter::kNoEntityId);

  SaveScene(false);

  entity_system_adapter()->OnDeactivate();

  gui_->Deactivate();

  NotifyExitEditor();
}

bool SceneLab::SaveScene(bool to_disk) {
  // Get a list of all filenames in the world.
  std::vector<GenericEntityId> entity_ids;
  if (!entity_system_adapter()->GetAllEntityIDs(&entity_ids)) {
    fplbase::LogInfo("Scene Lab: Couldn't get entity IDs.");
    return false;
  }
  // Deselect the selected entity.
  GenericEntityId prev_selected = selected_entity_;
  if (prev_selected != EntitySystemAdapter::kNoEntityId)
    SelectEntity(EntitySystemAdapter::kNoEntityId);

  // Divide up entity IDs by filename.
  std::string filename;
  std::unordered_map<std::string, std::vector<GenericEntityId>> ids_by_file;
  for (auto e = entity_ids.begin(); e != entity_ids.end(); ++e) {
    if (entity_system_adapter()->GetEntitySourceFile(*e, &filename)) {
      if (filename.length() == 0) {
        // Blank filename indicates save to a default file.
        filename = kDefaultEntityFile;
      }
      auto file = ids_by_file.find(filename);
      if (file == ids_by_file.end()) {
        ids_by_file[filename] = std::vector<GenericEntityId>();
        file = ids_by_file.find(filename);
      }
      file->second.push_back(*e);
    }
  }
  // Save entities in each file.
  for (auto iter = ids_by_file.begin(); iter != ids_by_file.end(); ++iter) {
    const std::string& filename = iter->first;
    if (filename.length() == 0) {
      // Skip blank filename.
      continue;
    }
    std::vector<uint8_t> output;
    if (entity_system_adapter()->SerializeEntities(iter->second, &output)) {
      if (to_disk) {
        // Write "output" to disk. Also write the JSON version.
        WriteEntityFile(filename, output);
      }
      entity_system_adapter()->OverrideFileCache(
          filename + "." + BinaryEntityFileExtension(), output);
    }
  }
  set_entities_modified(false);

  if (prev_selected != EntitySystemAdapter::kNoEntityId)
    SelectEntity(prev_selected);
  return true;
}

const char* SceneLab::BinaryEntityFileExtension() const {
  if (config_ != nullptr && config_->binary_entity_file_ext() != nullptr) {
    return config_->binary_entity_file_ext()->c_str();
  } else {
    return kDefaultBinaryEntityFileExtension;
  }
}

void SceneLab::WriteEntityFile(const std::string& filename,
                               const std::vector<uint8_t>& file_contents) {
  if (fplbase::SaveFile((filename + "." + BinaryEntityFileExtension()).c_str(),
                        file_contents.data(), file_contents.size())) {
    fplbase::LogInfo("Save (binary) to file '%s' successful.",
                     filename.c_str());
  } else {
    fplbase::LogError("Save (binary) to file '%s' failed.", filename.c_str());
  }
  // Now save to JSON file.
  // First load and parse the flatbuffer schema, then generate text.
  std::string schema_text;
  if (entity_system_adapter()->GetTextSchema(&schema_text)) {
    // Make a list of include paths that parser.Parse can parse.
    // char** with nullptr termination.
    std::unique_ptr<const char*> include_paths;
    size_t num_paths = config_->schema_include_paths()->size();
    include_paths.reset(new const char*[num_paths + 1]);
    for (size_t i = 0; i < num_paths; i++) {
      flatbuffers::uoffset_t index = static_cast<flatbuffers::uoffset_t>(i);
      include_paths.get()[i] =
          config_->schema_include_paths()->Get(index)->c_str();
    }
    include_paths.get()[num_paths] = nullptr;
    flatbuffers::Parser parser;
    if (parser.Parse(schema_text.c_str(), include_paths.get(),
                     config_->schema_file_text()->c_str())) {
      std::string json;
      parser.opts.strict_json = true;
      GenerateText(parser, file_contents.data(), &json);
      std::string json_path =
          (config_->json_output_directory()
               ? flatbuffers::ConCatPathFileName(
                     config_->json_output_directory()->str(), filename)
               : filename) +
          ".json";
      if (fplbase::SaveFile(json_path.c_str(), json)) {
        fplbase::LogInfo("Save (JSON) to file '%s' successful",
                         json_path.c_str());
      } else {
        fplbase::LogError("Save (JSON) to file '%s' failed.",
                          json_path.c_str());
      }
    } else {
      fplbase::LogError("Couldn't parse schema file: %s",
                        parser.error_.c_str());
    }
  } else {
    fplbase::LogError("No text schema loaded, can't save JSON file.");
  }
}

bool SceneLab::PreciseMovement() const {
  // When the shift key is held, use more precise movement.
  // TODO: would be better if we used precise movement by default, and
  //       transitioned to fast movement after the key has been held for
  //       a while.
  return (!gui_->InputCaptured() &&
          (controller_->KeyIsDown(fplbase::FPLK_LSHIFT) ||
           controller_->KeyIsDown(fplbase::FPLK_RSHIFT)));
}

vec3 SceneLab::GlobalFromHorizontal(float forward, float right, float up,
                                    const mathfu::vec3& plane_normal) const {
  return horizontal_forward_ * forward + horizontal_right_ * right +
         plane_normal * up;
}

bool SceneLab::IntersectRayToPlane(const vec3& ray_origin,
                                   const vec3& ray_direction,
                                   const vec3& point_on_plane,
                                   const vec3& plane_normal,
                                   vec3* intersection_point) {
  const float kEpsilon = 0.001f;
  vec3 ray_origin_to_plane = ray_origin - point_on_plane;
  float distance_from_ray_origin_to_plane =
      vec3::DotProduct(ray_origin_to_plane, plane_normal);
  // Nearest point on plane to ray origin =
  //     plane_normal . -distance_from_ray_origin_to_plane.
  float length_ratio = vec3::DotProduct(ray_direction, -plane_normal);
  // Ratio of diagonal / distance_from_ray_origin_to_plane.
  // float diagonal = length_ratio * distance_from_ray_origin_to_plane;
  if (distance_from_ray_origin_to_plane < kEpsilon &&
      distance_from_ray_origin_to_plane > -kEpsilon)
    *intersection_point = ray_origin_to_plane;
  else if (length_ratio < kEpsilon && length_ratio > -kEpsilon)
    return false;
  else
    *intersection_point =
        ray_origin +
        ray_direction * distance_from_ray_origin_to_plane * 1.0f / length_ratio;
  return true;
}

bool SceneLab::ProjectPointToPlane(const vec3& point_to_project,
                                   const vec3& point_on_plane,
                                   const vec3& plane_normal,
                                   vec3* point_projected) {
  // Try to intersect the point with the plane in both directions.
  if (IntersectRayToPlane(point_to_project, -plane_normal, point_on_plane,
                          plane_normal, point_projected)) {
    return true;
  }
  if (IntersectRayToPlane(point_to_project, plane_normal, point_on_plane,
                          plane_normal, point_projected)) {
    return true;
  }
  return false;  // Can't project the point for some reason.
}

vec3 SceneLab::GetMovement() const {
  if (gui_->InputCaptured()) return mathfu::kZeros3f;
  // Get a movement vector to move the user forward, up, or right.
  // Movement is always relative to the camera facing, but parallel to
  // ground.
  GenericCamera camera;
  entity_system_adapter()->GetCamera(&camera);

  float forward_speed = 0;
  float up_speed = 0;
  float right_speed = 0;
  const float move_speed =
      PreciseMovement()
          ? config_->camera_movement_speed() * config_->precise_movement_scale()
          : config_->camera_movement_speed();

  // TODO(jsimantov): make the specific keys configurable?
  if (controller_->KeyIsDown(fplbase::FPLK_w)) {
    forward_speed += move_speed;
  }
  if (controller_->KeyIsDown(fplbase::FPLK_s)) {
    forward_speed -= move_speed;
  }
  if (controller_->KeyIsDown(fplbase::FPLK_d)) {
    right_speed += move_speed;
  }
  if (controller_->KeyIsDown(fplbase::FPLK_a)) {
    right_speed -= move_speed;
  }
  if (gui_->lock_camera_height()) {
    // Camera movement is locked to the horizontal plane, so we need to have
    // a way for the user to move up and down.
    if (controller_->KeyIsDown(fplbase::FPLK_r)) {
      up_speed += move_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_f)) {
      up_speed -= move_speed;
    }
    // Translate the keypresses into movement parallel to the ground plane.
    return GlobalFromHorizontal(forward_speed, right_speed, up_speed,
                                camera.up);
  } else {
    // Just move relative to the direction of the camera.
    // forward-vector X up-vector = right-vector
    return camera.facing * forward_speed +
           vec3::CrossProduct(camera.facing, camera.up) * right_speed;
  }
}

bool SceneLab::ModifyTransformBasedOnInput(GenericTransform* transform) {
  GenericCamera camera;
  entity_system_adapter()->GetCamera(&camera);

  if (input_mode_ == kDragging) {
    ViewportSettings viewport;
    if (entity_system_adapter()->GetViewportSettings(&viewport)) {
      mathfu::vec2 pointer = controller_->GetPointer();
      vec3 mouse_ray_origin;
      vec3 mouse_ray_dir;
      if (controller_->ScreenPointToWorldRay(
              camera, viewport, pointer, renderer_->window_size(),
              &mouse_ray_origin, &mouse_ray_dir)) {
        vec3 intersect;

        if (IntersectRayToPlane(mouse_ray_origin, mouse_ray_dir, drag_point_,
                                drag_plane_normal_, &intersect)) {
          if (mouse_mode_ == kMoveHorizontal || mouse_mode_ == kMoveVertical) {
            vec3 new_pos = intersect + drag_offset_;
            transform->position = new_pos;
            return true;
          } else {
            // Rotation and scaling are both based on the relative position of
            // the mouse intersection point and the object's origin as projected
            // onto the drag plane.
            vec3 origin = transform->position;
            vec3 origin_on_plane;
            if (!ProjectPointToPlane(origin, drag_point_, drag_plane_normal_,
                                     &origin_on_plane)) {
              // For some reason we couldn't project the origin point onto the
              // drag plane. This is a weird situation; best just not modify
              // anything.
              return false;
            }
            if (mouse_mode_ == kRotateHorizontal ||
                mouse_mode_ == kRotateVertical) {
              // Figure out how much the drag point has rotated about the
              // object's origin since last frame, and add that to our current
              // rotation.
              vec3 new_rot = (intersect - origin).Normalized();
              vec3 old_rot = (drag_prev_intersect_ - origin).Normalized();
              vec3 cross = vec3::CrossProduct(old_rot, new_rot);
              float sin_a = vec3::DotProduct(cross, drag_plane_normal_) > 0
                                ? -cross.Length()
                                : cross.Length();
              float cos_a = vec3::DotProduct(old_rot, new_rot);
              float angle = atan2(sin_a, cos_a);
              drag_prev_intersect_ = intersect;

              // Apply 'angle' with axis 'drag_plane_normal_' to existing
              // orientation.
              mathfu::quat orientation = transform->orientation;
              orientation = orientation * mathfu::quat::FromAngleAxis(
                                              angle, drag_plane_normal_);
              transform->orientation = orientation;
              return true;

            } else if (mouse_mode_ == kScaleX || mouse_mode_ == kScaleY ||
                       mouse_mode_ == kScaleZ || mouse_mode_ == kScaleAll) {
              float old_offset = (drag_point_ - origin).Length();
              float new_offset = (intersect - origin).Length();
              if (old_offset == 0) return false;  // Avoid division by zero.
              // Use the ratio of the original and current distance between
              // mouse
              // cursor and object origin to determine scale. Direction is
              // ignored.
              float scale = new_offset / old_offset;
              float xscale =
                  (mouse_mode_ == kScaleX || mouse_mode_ == kScaleAll) ? scale
                                                                       : 1;
              float yscale =
                  (mouse_mode_ == kScaleY || mouse_mode_ == kScaleAll) ? scale
                                                                       : 1;
              float zscale =
                  (mouse_mode_ == kScaleZ || mouse_mode_ == kScaleAll) ? scale
                                                                       : 1;
              transform->scale = mathfu::vec3(drag_orig_scale_.x() * xscale,
                                              drag_orig_scale_.y() * yscale,
                                              drag_orig_scale_.z() * zscale);
              return true;
            }
          }
        }
      }
    }
  } else {
    // Not dragging, use keyboard keys instead.
    if (gui_->InputCaptured()) return false;

    // IJKL = move x/y axis
    float fwd_speed = 0, right_speed = 0, up_speed = 0;
    float roll_speed = 0, pitch_speed = 0, yaw_speed = 0;
    float scale_speed = 1;

    // When the shift key is held, use more precise movement.
    // TODO: would be better if we used precise movement by default, and
    //       transitioned to fast movement after the key has been held for
    //       a while.
    const float movement_scale =
        PreciseMovement() ? config_->precise_movement_scale() : 1.0f;
    const float move_speed = movement_scale * config_->object_movement_speed();
    const float angular_speed =
        movement_scale * config_->object_angular_speed();

    if (controller_->KeyIsDown(fplbase::FPLK_i)) {
      fwd_speed += move_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_k)) {
      fwd_speed -= move_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_j)) {
      right_speed -= move_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_l)) {
      right_speed += move_speed;
    }
    // P; = move z axis
    if (controller_->KeyIsDown(fplbase::FPLK_p)) {
      up_speed += move_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_SEMICOLON)) {
      up_speed -= move_speed;
    }
    // UO = roll
    if (controller_->KeyIsDown(fplbase::FPLK_u)) {
      roll_speed += angular_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_o)) {
      roll_speed -= angular_speed;
    }
    // YH = pitch
    if (controller_->KeyIsDown(fplbase::FPLK_y)) {
      pitch_speed += angular_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_h)) {
      pitch_speed -= angular_speed;
    }
    // NM = yaw
    if (controller_->KeyIsDown(fplbase::FPLK_n)) {
      yaw_speed += angular_speed;
    }
    if (controller_->KeyIsDown(fplbase::FPLK_m)) {
      yaw_speed -= angular_speed;
    }
    // +- = scale
    if (controller_->KeyIsDown(fplbase::FPLK_EQUALS)) {
      scale_speed = config_->object_scale_speed();
    } else if (controller_->KeyIsDown(fplbase::FPLK_MINUS)) {
      scale_speed = 1.0f / config_->object_scale_speed();
    }
    const vec3 new_position =
        transform->position +
        GlobalFromHorizontal(fwd_speed, right_speed, up_speed, camera.up);

    vec3 new_orientation = transform->orientation.ToEulerAngles() +
                           vec3(pitch_speed, roll_speed, yaw_speed);

    vec3 new_scale = transform->scale;
    if (controller_->KeyIsDown(fplbase::FPLK_0) &&
        controller_->KeyIsDown(fplbase::FPLK_LCTRL)) {
      new_scale = mathfu::kOnes3f;
      scale_speed = 0;  // to trigger modified = true
    } else {
      new_scale = scale_speed * new_scale;
    }
    bool modified = false;
    if (fwd_speed != 0 || right_speed != 0 || up_speed != 0) {
      modified = true;
      transform->position = new_position;
    }
    if (yaw_speed != 0 || roll_speed != 0 || pitch_speed != 0) {
      modified = true;
      transform->orientation = mathfu::quat::FromEulerAngles(new_orientation);
    }
    if (scale_speed != 1) {
      modified = true;
      transform->scale = new_scale;
    }
    if (modified) return true;
  }
  return false;
}

void SceneLab::RequestExit() {
  if (input_mode_ != kDragging) {
    if (gui_->CanDeselectEntity()) {
      exit_requested_ = true;
      exit_ready_ = false;
      if (gui_->CanExit()) {
        exit_ready_ = true;
      } else {
        if (input_mode_ != kEditing) {
          input_mode_ = kEditing;
        }
      }
    }
  }
}

void SceneLab::AbortExit() { exit_requested_ = false; }

bool SceneLab::IsReadyToExit() { return exit_requested_ && exit_ready_; }

void SceneLab::AddOnEnterEditorCallback(EditorCallback callback) {
  on_enter_editor_callbacks_.push_back(callback);
}

void SceneLab::AddOnExitEditorCallback(EditorCallback callback) {
  on_exit_editor_callbacks_.push_back(callback);
}

void SceneLab::AddOnCreateEntityCallback(EntityCallback callback) {
  on_create_entity_callbacks_.push_back(callback);
}

void SceneLab::AddOnUpdateEntityCallback(EntityCallback callback) {
  on_update_entity_callbacks_.push_back(callback);
}

void SceneLab::AddOnDeleteEntityCallback(EntityCallback callback) {
  on_delete_entity_callbacks_.push_back(callback);
}

}  // namespace editor
