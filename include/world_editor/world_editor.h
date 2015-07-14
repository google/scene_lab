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

#ifndef FPL_WORLD_EDITOR_H_
#define FPL_WORLD_EDITOR_H_

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "component_library/camera_interface.h"
#include "component_library/meta.h"
#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "component_library/transform.h"
#include "component_library/entity_factory.h"
#include "editor_components_generated.h"
#include "entity/component_interface.h"
#include "entity/entity_manager.h"
#include "event/event_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "mathfu/vector_3.h"
#include "world_editor/edit_options.h"
#include "world_editor/editor_event.h"
#include "world_editor/editor_controller.h"
#include "world_editor_config_generated.h"

namespace fpl {
namespace editor {

class WorldEditor {
 public:
  void Initialize(const WorldEditorConfig* config, InputSystem* input_system,
                  entity::EntityManager* entity_manager,
                  event::EventManager* event_manager,
                  component_library::EntityFactory* entity_factory);
  void AdvanceFrame(WorldTime delta_time);
  void Render(Renderer* renderer);
  void Activate();
  void Deactivate();
  void SetInitialCamera(const CameraInterface& initial_camera);
  void SetCamera(std::unique_ptr<CameraInterface> camera) {
    camera_ = std::move(camera);
  }

  void RegisterComponent(entity::ComponentInterface* component);

  const CameraInterface* GetCamera() const { return camera_.get(); }

  void SelectEntity(const entity::EntityRef& entity_ref);

  void SaveWorld();
  void SaveEntitiesInFile(const std::string& filename);

  void AddComponentToUpdate(entity::ComponentId component_id) {
    components_to_update_.push_back(component_id);
  }

 private:
  enum { kMoving, kEditing } input_mode_;

  // return true if we should be moving the camera and objects slowly.
  bool PreciseMovement() const;

  // return a global vector from camera coordinates relative to the horizontal
  // plane.
  mathfu::vec3 GlobalFromHorizontal(float forward, float right, float up) const;

  // get camera movement via W-A-S-D
  mathfu::vec3 GetMovement() const;

  entity::EntityRef DuplicateEntity(entity::EntityRef& entity);
  void DestroyEntity(entity::EntityRef& entity);
  void HighlightEntity(const entity::EntityRef& entity, float tint);
  void NotifyEntityUpdated(const entity::EntityRef& entity) const;
  void NotifyEntityDeleted(const entity::EntityRef& entity) const;

  // returns true if the transform was modified
  bool ModifyTransformBasedOnInput(TransformDef* transform);

  void LoadSchemaFiles();

  const WorldEditorConfig* config_;
  InputSystem* input_system_;
  entity::EntityManager* entity_manager_;
  event::EventManager* event_manager_;
  component_library::EntityFactory* entity_factory_;
  // Which entity are we currently editing?
  entity::EntityRef selected_entity_;

  // Temporary solution to let us cycle through all entities.
  std::unique_ptr<entity::EntityManager::EntityStorageContainer::Iterator>
      entity_cycler_;

  // For storing the FlatBuffers schema we use for exporting.
  std::string schema_data_;
  std::string schema_text_;

  bool previous_relative_mouse_mode;

  std::vector<entity::ComponentId> components_to_update_;
  std::unique_ptr<EditorController> controller_;
  std::unique_ptr<CameraInterface> camera_;

  // Camera angles, projected onto the horizontal plane, as defined by the
  // camera's `up()` direction.
  mathfu::vec3 horizontal_forward_;
  mathfu::vec3 horizontal_right_;
};

}  // namespace editor
}  // namespace fpl

#endif  // FPL_WORLD_EDITOR_H_
