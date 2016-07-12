// Copyright 2016 Google Inc. All rights reserved.
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

#ifndef SCENE_LAB_CORGI_CORGI_ADAPTER_H_
#define SCENE_LAB_CORGI_CORGI_ADAPTER_H_

#include "corgi/entity_manager.h"
#include "corgi_component_library/camera_interface.h"
#include "corgi_component_library/entity_factory.h"
#include "mathfu/glsl_mappings.h"
#include "scene_lab/entity_system_adapter.h"
#include "scene_lab/scene_lab.h"

namespace scene_lab {
class SceneLab;
}  // namespace scene_lab

namespace scene_lab_corgi {

// For CorgiAdapter:
// * GenericEntityId is the entity_id in the entity's MetaData.
// * GenericComponentId is a string representation of the component ID number.

class CorgiAdapter : public scene_lab::EntitySystemAdapter {
 public:
  // Allow these to be easily accessed from this namespace.
  typedef scene_lab::GenericEntityId GenericEntityId;
  typedef scene_lab::GenericComponentId GenericComponentId;
  typedef scene_lab::GenericPrototypeId GenericPrototypeId;
  typedef scene_lab::GenericCamera GenericCamera;
  typedef scene_lab::GenericTransform GenericTransform;
  typedef scene_lab::ViewportSettings ViewportSettings;

  CorgiAdapter(scene_lab::SceneLab* scene_lab,
               corgi::EntityManager* entity_manager);
  virtual ~CorgiAdapter() {}

  /// Give CorgiAdapter a camera that it can use. If you don't call this, it
  /// will create its own BasicCamera instead.
  ///
  /// CorgiAdapter will take over ownership of `camera`.
  void SetCorgiCamera(std::unique_ptr<corgi::CameraInterface> camera) {
    camera_ = std::move(camera);
  }

  virtual void AdvanceFrame(double delta_seconds);

  virtual void OnActivate();

  virtual void OnDeactivate();

  virtual bool EntityExists(const GenericEntityId& id);

  virtual bool GetEntityTransform(const GenericEntityId& id,
                                  GenericTransform* transform_output);

  virtual bool SetEntityTransform(const GenericEntityId& id,
                                  const GenericTransform& transform);

  virtual bool GetEntityChildren(const GenericEntityId& id,
                                 std::vector<GenericEntityId>* children_out);

  virtual bool GetEntityParent(const GenericEntityId& id,
                               GenericEntityId* parent_out);

  virtual bool SetEntityParent(const GenericEntityId& child,
                               const GenericEntityId& parent);

  virtual bool GetCamera(GenericCamera* camera);

  virtual bool SetCamera(const GenericCamera& camera);

  virtual bool GetViewportSettings(ViewportSettings* viewport);

  virtual bool DuplicateEntity(const GenericEntityId& id,
                               GenericEntityId* new_id_output);

  virtual bool CreateEntity(GenericEntityId* new_id_output);

  virtual bool CreateEntityFromPrototype(const GenericPrototypeId& prototype,
                                         GenericEntityId* new_id_output);

  virtual bool DeleteEntity(const GenericEntityId& id);

  virtual bool SetEntityHighlighted(const GenericEntityId& id,
                                    bool is_highlighted);

  virtual bool DebugDrawPhysics(const GenericEntityId& id);

  virtual bool GetRayIntersection(const mathfu::vec3& start_point,
                                  const mathfu::vec3& direction,
                                  GenericEntityId* entity_output,
                                  mathfu::vec3* intersection_point_output);

  virtual bool CycleEntities(int direction, GenericEntityId* next_entity);

  virtual bool GetAllEntityIDs(std::vector<GenericEntityId>* ids_out);

  virtual bool GetAllPrototypeIDs(std::vector<GenericPrototypeId>* ids_out);

  virtual bool GetEntityName(const GenericEntityId& id, std::string* name_out) {
    /// Use the ID string.
    if (name_out != nullptr) *name_out = static_cast<std::string>(id);
    return true;
  }

  virtual bool GetEntityDescription(const GenericEntityId& id,
                                    std::string* description_out);

  virtual bool GetEntitySourceFile(const GenericEntityId& id,
                                   std::string* source_file);

  virtual bool GetSchema(const reflection::Schema** schema_out);

  virtual bool GetTextSchema(std::string* schema_out);

  virtual bool GetTableObject(const GenericComponentId& id,
                              const reflection::Object** table_out);

  virtual bool GetEntityComponentList(
      const GenericEntityId& id,
      std::vector<GenericComponentId>* components_out);

  virtual void GetFullComponentList(
      std::vector<GenericComponentId>* components_out);

  virtual bool IsEntityComponentFromPrototype(
      const GenericEntityId& entity, const GenericComponentId& component);

  virtual bool SerializeEntities(const std::vector<GenericEntityId>& id,
                                 std::vector<uint8_t>* buffer_out);

  virtual bool SerializeEntityComponent(const GenericEntityId& entity_id,
                                        const GenericComponentId& component,
                                        flatbuffers::unique_ptr_t* data_out);

  virtual bool DeserializeEntityComponent(const GenericEntityId& entity_id,
                                          const GenericComponentId& component,
                                          const uint8_t* data);

  virtual void OverrideFileCache(const std::string& filename,
                                 const std::vector<uint8_t>& data);

  /// Add a component to the list of components to update each frame.
  ///
  /// While Scene Lab is activated, you should no longer be calling
  /// EntityManager::UpdateComponents(); you should let CorgiAdapter update only
  /// the components it cares about. If you have any components you are sure you
  /// also want updated while editing the scene, add them to the list by calling
  /// this function.
  void AddComponentToUpdate(corgi::ComponentId component_id) {
    components_to_update_.push_back(component_id);
  }

  corgi::CameraInterface* GetCorgiCamera() {
    if (camera_ == nullptr) CreateDefaultCamera();
    return camera_.get();
  }

  /// Convert a Scene Lab entity ID to a Corgi entity reference.
  /// This is public so that Corgi (particularly EditOptionsComponent) can talk
  /// to Scene Lab properly.
  corgi::EntityRef GetEntityRef(const GenericEntityId& id) const;

  /// Convert a Corgi entity reference to a Scene Lab entity ID.
  /// This is public so that Corgi components can talk to Scene Lab properly.
  GenericEntityId GetEntityId(const corgi::EntityRef& entity) const;

  /// Convert a Scene Lab component ID to a Corgi component ID.
  /// This is public so that Corgi components can talk to Scene Lab properly.
  corgi::ComponentId GetCorgiComponentId(const GenericComponentId& id) const;

  /// Convert a Corgi component ID to a Scene Lab component ID.
  /// This is public so that Corgi components can talk to Scene Lab properly.
  GenericComponentId GetGenericComponentId(corgi::ComponentId c) const;

  /// Get a pointer back to our Scene Lab instance.
  scene_lab::SceneLab* scene_lab() const { return scene_lab_; }

 private:
  /// Highlight a node with the given tint, and its children with slightly less
  /// tint. Returns true if at least one node's tint was set, or false if
  /// no tints were set because the entity nor its children are renderable.
  bool HighlightEntity(const corgi::EntityRef& id, float tint);

  void CreateDefaultCamera();

  scene_lab::SceneLab* scene_lab_;

  std::unique_ptr<corgi::CameraInterface> camera_;
  corgi::EntityManager* entity_manager_;
  corgi::component_library::EntityFactory* entity_factory_;
  fplbase::Renderer* renderer_;
  std::vector<corgi::ComponentId> components_to_update_;
  corgi::EntityManager::EntityStorageContainer::Iterator entity_cycler_;

  // For storing the FlatBuffers schema we use for exporting.
  std::string schema_data_;
  std::string schema_text_;

  float rendermesh_culling_distance_squared_;
};

}  // namespace scene_lab_corgi

#endif  // SCENE_LAB_CORGI_CORGI_ADAPTER_H_
