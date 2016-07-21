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

#include "scene_lab/corgi/corgi_adapter.h"

#include <string>
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/meta.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/rendermesh.h"
#include "corgi_component_library/transform.h"
#include "mathfu/glsl_mappings.h"
#include "scene_lab/basic_camera.h"
#include "scene_lab/corgi/edit_options.h"

namespace scene_lab_corgi {

using corgi::component_library::CommonServicesComponent;
using corgi::component_library::MetaComponent;
using corgi::component_library::MetaData;
using corgi::component_library::PhysicsComponent;
using corgi::component_library::PhysicsData;
using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;
using corgi::component_library::RenderMeshComponent;
using corgi::component_library::RenderMeshData;
using scene_lab::SceneLab;
using scene_lab::GenericEntityId;
using scene_lab::GenericComponentId;

CorgiAdapter::CorgiAdapter(SceneLab* scene_lab,
                           corgi::EntityManager* entity_manager)
    : scene_lab_(scene_lab),
      entity_manager_(entity_manager),
      entity_cycler_(entity_manager->begin()) {
  auto services = entity_manager_->GetComponent<CommonServicesComponent>();
  renderer_ = services->renderer();
  entity_factory_ = services->entity_factory();
  rendermesh_culling_distance_squared_ = 0;

  auto edit_options = entity_manager_->GetComponent<EditOptionsComponent>();
  if (edit_options) {
    edit_options->SetSceneLabCallbacks(this);
  }

  const char* schema_file_text =
      scene_lab_->config()->schema_file_text()->c_str();
  const char* schema_file_binary =
      scene_lab_->config()->schema_file_binary()->c_str();

  if (!fplbase::LoadFile(schema_file_binary, &schema_data_)) {
    fplbase::LogError("CorgiAdapter: Failed to open binary schema file: %s",
                      schema_file_binary);
  }
  auto schema = reflection::GetSchema(schema_data_.c_str());
  if (schema != nullptr) {
    fplbase::LogInfo("CorgiAdapter: Binary schema %s loaded",
                     schema_file_binary);
  }
  if (fplbase::LoadFile(schema_file_text, &schema_text_)) {
    fplbase::LogInfo("CorgiAdapter: Text schema %s loaded", schema_file_text);
  }

  scene_lab_->gui()->SetShowComponentDataView(
      GetGenericComponentId(MetaComponent::GetComponentId()), true);
  scene_lab_->gui()->SetShowComponentDataView(
      GetGenericComponentId(TransformComponent::GetComponentId()), true);

  AddComponentToUpdate(TransformComponent::GetComponentId());
}

void CorgiAdapter::AdvanceFrame(double delta_seconds) {
  corgi::WorldTime delta_time = static_cast<corgi::WorldTime>(
      delta_seconds * corgi::kMillisecondsPerSecond);
  (void)delta_time;

  auto transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  transform_component->PostLoadFixup();

  // Any components we specifically still want to update should be updated here.
  for (size_t i = 0; i < components_to_update_.size(); i++) {
    entity_manager_->GetComponent(components_to_update_[i])
        ->UpdateAllEntities(0);
  }

  entity_manager_->DeleteMarkedEntities();
}

void CorgiAdapter::OnActivate() {
  if (camera_ == nullptr) {
    CreateDefaultCamera();
  }
  // Disable distance culling, if enabled.
  auto render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  if (render_mesh_component != nullptr) {
    rendermesh_culling_distance_squared_ =
        render_mesh_component->culling_distance_squared();
    // Only cull past the far clipping plane.
    render_mesh_component->set_culling_distance_squared(
        camera_->viewport_far_plane() * camera_->viewport_far_plane());
  }
}

void CorgiAdapter::OnDeactivate() {
  // Restore previous distance culling setting.
  auto render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  if (render_mesh_component != nullptr) {
    render_mesh_component->set_culling_distance_squared(
        rendermesh_culling_distance_squared_);
  }

  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (physics_component != nullptr) {
    physics_component->AwakenAllEntities();
  }
}

bool CorgiAdapter::EntityExists(const GenericEntityId& id) {
  return GetEntityRef(id);
}

bool CorgiAdapter::GetEntityTransform(const GenericEntityId& id,
                                      GenericTransform* transform) {
  if (!EntityExists(id)) return false;
  auto transform_data =
      entity_manager_->GetComponentData<TransformData>(GetEntityRef(id));

  if (transform_data == nullptr) {
    return false;
  }
  transform->position = transform_data->position;
  transform->orientation = transform_data->orientation;
  transform->scale = transform_data->scale;
  return true;
}

bool CorgiAdapter::SetEntityTransform(const GenericEntityId& id,
                                      const GenericTransform& transform) {
  if (!EntityExists(id)) return false;
  auto transform_component =
      entity_manager_->GetComponent<TransformComponent>();

  corgi::EntityRef entity = GetEntityRef(id);
  // Get the transform assigned to this entity, adding one if needed.
  auto transform_data = transform_component->AddEntity(entity);

  if (transform_data == nullptr) {
    // Failed to add the transform to the entity.
    return false;
  }
  transform_data->position = transform.position;
  transform_data->orientation = transform.orientation;
  transform_data->scale = transform.scale;

  if (entity_manager_->GetComponentData<PhysicsData>(entity)) {
    PhysicsComponent* physics =
        entity_manager_->GetComponent<PhysicsComponent>();
    physics->UpdatePhysicsFromTransform(entity);
    // Workaround for an issue with the physics library where modifying
    // a raycast physics volume causes raycasts to stop working on it.
    physics->DisablePhysics(entity);
    physics->EnablePhysics(entity);
  }
  return true;
}

bool CorgiAdapter::GetEntityChildren(
    const GenericEntityId& id, std::vector<GenericEntityId>* children_out) {
  if (!EntityExists(id)) return false;
  auto transform_data =
      entity_manager_->GetComponentData<TransformData>(GetEntityRef(id));

  if (transform_data == nullptr) {
    return false;
  }

  for (auto iter = transform_data->children.begin();
       iter != transform_data->children.end(); ++iter) {
    if (children_out != nullptr)
      children_out->push_back(GetEntityId(iter->owner));
  }
  return true;
}

bool CorgiAdapter::GetEntityParent(const GenericEntityId& id,
                                   GenericEntityId* parent_out) {
  if (!EntityExists(id)) return false;
  auto transform_data =
      entity_manager_->GetComponentData<TransformData>(GetEntityRef(id));

  if (transform_data == nullptr) {
    return false;
  }
  if (parent_out != nullptr) {
    if (transform_data->parent) {
      *parent_out = GetEntityId(transform_data->parent);
    } else {
      *parent_out = kNoEntityId;
    }
  }
  return true;
}

bool CorgiAdapter::SetEntityParent(const GenericEntityId& child,
                                   const GenericEntityId& parent) {
  if (!EntityExists(child)) return false;
  corgi::EntityRef child_entity = GetEntityRef(child);
  auto transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  if (transform_component->GetComponentData(child_entity) == nullptr)
    return false;

  if (parent == kNoEntityId) {
    // No parent specified, just remove the current parent, if any.
    GenericEntityId current_parent = kNoEntityId;
    GetEntityParent(child, &current_parent);
    if (current_parent != kNoEntityId) {
      transform_component->RemoveChild(child_entity);
    }
  } else {
    // parent is an entity, check if valid.
    if (!EntityExists(parent)) return false;
    corgi::EntityRef parent_entity = GetEntityRef(parent);
    if (transform_component->GetComponentData(parent_entity) == nullptr)
      return false;

    transform_component->AddChild(child_entity, parent_entity);
  }
  return true;
}

bool CorgiAdapter::GetCamera(GenericCamera* camera_out) {
  if (camera_ == nullptr) {
    CreateDefaultCamera();
  }
  if (camera_out != nullptr) {
    camera_out->position = camera_->position();
    camera_out->facing = camera_->facing();
    camera_out->up = camera_->up();
  }
  return true;
}

bool CorgiAdapter::SetCamera(const GenericCamera& camera_in) {
  if (camera_ == nullptr) {
    CreateDefaultCamera();
  }
  camera_->set_position(camera_in.position);
  if (camera_in.facing.LengthSquared() != 0) {
    camera_->set_facing(camera_in.facing);
  }
  if (camera_in.up.LengthSquared() != 0) {
    camera_->set_up(camera_in.up);
  }
  return true;
}

bool CorgiAdapter::GetViewportSettings(ViewportSettings* viewport_out) {
  if (camera_ == nullptr) {
    CreateDefaultCamera();
  }
  if (viewport_out != nullptr) {
    viewport_out->vertical_angle = camera_->viewport_angle();
    viewport_out->aspect_ratio =
        camera_->viewport_resolution().x() / camera_->viewport_resolution().y();
  }
  return true;
}

bool CorgiAdapter::DuplicateEntity(const GenericEntityId& id,
                                   GenericEntityId* new_id) {
  corgi::EntityRef entity = GetEntityRef(id);

  std::vector<uint8_t> entity_serialized;
  if (!entity_factory_->SerializeEntity(entity, entity_manager_,
                                        &entity_serialized)) {
    fplbase::LogError("DuplicateEntity: Couldn't serialize entity");
    return false;
  }
  std::vector<std::vector<uint8_t>> entity_defs;
  entity_defs.push_back(entity_serialized);
  std::vector<uint8_t> entity_list;
  if (!entity_factory_->SerializeEntityList(entity_defs, &entity_list)) {
    fplbase::LogError("DuplicateEntity: Couldn't create entity list");
    return false;
  }
  std::vector<corgi::EntityRef> entities_created;
  if (entity_factory_->LoadEntityListFromMemory(
          entity_list.data(), entity_manager_, &entities_created) > 0) {
    // We created some new duplicate entities! (most likely exactly one.)  We
    // need to remove their entity IDs since otherwise they will have
    // duplicate entity IDs to the one we created.  We also need to make sure
    // the new entity IDs are marked with the same source file as the old.

    for (size_t i = 0; i < entities_created.size(); i++) {
      corgi::EntityRef& new_entity = entities_created[i];
      MetaData* old_editor_data =
          entity_manager_->GetComponentData<MetaData>(entity);
      MetaData* editor_data =
          entity_manager_->GetComponentData<MetaData>(new_entity);
      if (editor_data != nullptr) {
        editor_data->entity_id = "";
        if (old_editor_data != nullptr)
          editor_data->source_file = old_editor_data->source_file;
      }
    }
    entity_manager_->GetComponent<TransformComponent>()->PostLoadFixup();
    for (size_t i = 0; i < entities_created.size(); i++) {
      scene_lab_->NotifyCreateEntity(GetEntityId(entities_created[i]));
    }
    *new_id = GetEntityId(entities_created[0]);
    return true;
  }
  return false;
}

bool CorgiAdapter::CreateEntity(GenericEntityId* new_id_output) {
  corgi::EntityRef new_entity = entity_manager_->AllocateNewEntity();
  if (new_entity) {
    if (new_id_output != nullptr) {
      *new_id_output = GetEntityId(new_entity);
    }
    return true;
  } else {
    return false;
  }
}

bool CorgiAdapter::CreateEntityFromPrototype(
    const GenericPrototypeId& prototype, GenericEntityId* new_id_output) {
  std::string prototype_str = static_cast<std::string>(prototype);
  corgi::EntityRef new_entity = entity_factory_->CreateEntityFromPrototype(
      prototype_str.c_str(), entity_manager_);
  if (new_entity) {
    if (new_id_output != nullptr) {
      *new_id_output = GetEntityId(new_entity);
    }
    return true;
  } else {
    return false;  // Couldn't create.
  }
}

bool CorgiAdapter::DeleteEntity(const GenericEntityId& id) {
  if (!EntityExists(id)) return false;
  corgi::EntityRef entity = GetEntityRef(id);
  entity_manager_->DeleteEntity(entity);
  return true;
}

bool CorgiAdapter::SetEntityHighlighted(const GenericEntityId& id,
                                        bool is_highlighted) {
  if (!EntityExists(id)) return false;
  return HighlightEntity(GetEntityRef(id), is_highlighted ? 2.0f : 1.0f);
}

bool CorgiAdapter::DebugDrawPhysics(const GenericEntityId& id) {
  if (EntityExists(id)) {
    corgi::EntityRef entity = GetEntityRef(id);
    if (entity) {
      auto physics = entity_manager_->GetComponent<PhysicsComponent>();
      mathfu::mat4 cam = camera_->GetTransformMatrix();
      physics->DebugDrawObject(renderer_, cam, entity,
                               mathfu::vec3(1.0f, 0.5f, 0.5f));
      return true;
    }
  }
  return false;
}

bool CorgiAdapter::GetRayIntersection(const mathfu::vec3& start_point,
                                      const mathfu::vec3& direction,
                                      GenericEntityId* entity_output,
                                      mathfu::vec3* intersection_point_output) {
  if (camera_ == nullptr) return false;

  mathfu::vec3 intersection_point;
  mathfu::vec3 start = start_point;
  float distance = camera_->viewport_far_plane();
  mathfu::vec3 end = start + distance * direction;
  corgi::EntityRef result =
      entity_manager_->GetComponent<PhysicsComponent>()->RaycastSingle(
          start, end, &intersection_point);
  if (result) {
    if (entity_output != nullptr) *entity_output = GetEntityId(result);
    if (intersection_point_output != nullptr)
      *intersection_point_output = intersection_point;
    return true;
  }
  return false;
}

bool CorgiAdapter::CycleEntities(int direction, GenericEntityId* next_entity) {
  corgi::EntityRef current = entity_cycler_.ToReference();
  if (!current) entity_cycler_ = entity_manager_->begin();

  if (direction == 0) {
    // Reset to the beginning, ignoring current_entity.
    entity_cycler_ = entity_manager_->begin();
  } else if (direction > 0) {
    // Cycle forward N entities.
    for (int i = 0; i < direction; i++) {
      if (entity_cycler_ != entity_manager_->end()) {
        entity_cycler_++;
      }
      if (entity_cycler_ == entity_manager_->end()) {
        entity_cycler_ = entity_manager_->begin();
      }
    }
  } else {  // direction < 0
    // Cycle backward N entities.
    for (int i = 0; i < -direction; i++) {
      if (entity_cycler_ == entity_manager_->begin()) {
        entity_cycler_ = entity_manager_->end();
      }
      entity_cycler_--;
    }
  }
  // fplbase::LogInfo("Cycled to entity: %x", entity_cycler_);
  if (next_entity != nullptr && entity_cycler_.ToReference()) {
    *next_entity = GetEntityId(entity_cycler_.ToReference());
  }
  return true;
}

bool CorgiAdapter::GetAllEntityIDs(std::vector<GenericEntityId>* ids_out) {
  if (ids_out != nullptr) ids_out->clear();
  for (auto i = entity_manager_->begin(); i != entity_manager_->end(); ++i) {
    corgi::EntityRef entity = i.ToReference();
    if (ids_out != nullptr) ids_out->push_back(GetEntityId(entity));
  }
  return true;
}

bool CorgiAdapter::GetAllPrototypeIDs(
    std::vector<GenericPrototypeId>* ids_out) {
  auto prototype_data = entity_factory_->prototype_data();
  if (ids_out != nullptr) ids_out->clear();
  for (auto it = prototype_data.begin(); it != prototype_data.end(); ++it) {
    if (ids_out != nullptr) ids_out->push_back(it->first);
  }
  return true;
}

bool CorgiAdapter::GetEntityDescription(const GenericEntityId& id,
                                        std::string* description) {
  if (!EntityExists(id)) return false;
  corgi::EntityRef entity = GetEntityRef(id);
  if (!entity) return false;

  auto editor_data = entity_manager_->GetComponentData<MetaData>(entity);
  if (editor_data == nullptr) return true;
  if (editor_data->prototype.length() > 0 && description != nullptr) {
    *description = editor_data->prototype;
  }
  return editor_data->prototype.length() > 0;
}

bool CorgiAdapter::GetEntitySourceFile(const GenericEntityId& id,
                                       std::string* source_file) {
  // If the entity is not found, return false, meaning don't save.
  if (source_file != nullptr) *source_file = std::string();
  if (!EntityExists(id)) return false;
  corgi::EntityRef entity = GetEntityRef(id);
  if (!entity) return false;

  auto editor_data = entity_manager_->GetComponentData<MetaData>(entity);
  if (editor_data == nullptr) return true;
  if (source_file != nullptr) {
    *source_file = editor_data->source_file;
  }
  // Entities with a blank filename should not be saved, so return false in that
  // case.
  return editor_data->source_file.length() > 0;
}

bool CorgiAdapter::GetSchema(const reflection::Schema** schema_out) {
  if (schema_data_.length() > 0) {
    if (schema_out != nullptr) {
      *schema_out = reflection::GetSchema(schema_data_.c_str());
    }
    return true;
  }
  return false;
}

bool CorgiAdapter::GetTextSchema(std::string* schema_out) {
  if (schema_text_.length() > 0) {
    if (schema_out != nullptr) *schema_out = schema_text_;
    return true;
  }
  return false;
}

bool CorgiAdapter::GetTableObject(const GenericComponentId& id,
                                  const reflection::Object** table_out) {
  const reflection::Schema* schema;
  if (!GetSchema(&schema)) return false;
  // GenericComponentId is the table name.
  const char* table_name =
      entity_factory_->ComponentIdToTableName(GetCorgiComponentId(id));
  if (table_name == nullptr) return false;

  const reflection::Object* obj = schema->objects()->LookupByKey(table_name);
  if (table_out != nullptr) *table_out = obj;
  return (obj != nullptr);
}

bool CorgiAdapter::GetEntityComponentList(
    const GenericEntityId& id,
    std::vector<GenericComponentId>* components_out) {
  if (!EntityExists(id)) return false;
  corgi::EntityRef entity = GetEntityRef(id);
  if (!entity) return false;

  if (components_out != nullptr) components_out->clear();
  for (corgi::ComponentId i = 0; i < entity_manager_->ComponentCount(); i++) {
    if (i != corgi::kInvalidComponent &&
        entity_manager_->GetComponent(i)->GetComponentDataAsVoid(entity) !=
            nullptr) {
      if (components_out != nullptr) {
        components_out->push_back(GetGenericComponentId(i));
      }
    }
  }
  return true;
}

void CorgiAdapter::GetFullComponentList(
    std::vector<GenericComponentId>* components_out) {
  if (components_out != nullptr) components_out->clear();
  for (corgi::ComponentId i = 0; i < entity_manager_->ComponentCount(); i++) {
    if (i != corgi::kInvalidComponent) {
      if (components_out != nullptr) {
        components_out->push_back(GetGenericComponentId(i));
      }
    }
  }
}

bool CorgiAdapter::IsEntityComponentFromPrototype(
    const GenericEntityId& entity_id, const GenericComponentId& component_id) {
  if (!EntityExists(entity_id)) return false;
  corgi::EntityRef entity = GetEntityRef(entity_id);
  corgi::ComponentId cid = GetCorgiComponentId(component_id);
  if (!entity || cid == corgi::kInvalidComponent) return false;

  auto meta_data = entity_manager_->GetComponentData<MetaData>(entity);
  return meta_data ? (meta_data->components_from_prototype.find(cid) !=
                      meta_data->components_from_prototype.end())
                   : false;
}

bool CorgiAdapter::SerializeEntities(
    const std::vector<GenericEntityId>& id_list,
    std::vector<uint8_t>* buffer_out) {
  std::vector<std::vector<uint8_t>> entities_serialized;
  for (auto id = id_list.begin(); id != id_list.end(); ++id) {
    if (!EntityExists(*id)) continue;
    corgi::EntityRef entity = GetEntityRef(*id);
    if (!entity) continue;
    entities_serialized.push_back(std::vector<uint8_t>());
    entity_factory_->SerializeEntity(entity, entity_manager_,
                                     &entities_serialized.back());
  }
  if (buffer_out != nullptr) {
    if (!entity_factory_->SerializeEntityList(entities_serialized,
                                              buffer_out)) {
      fplbase::LogError("CorgiAdapter: Couldn't serialize entity list.");
      return false;
    }
  }
  return true;
}

bool CorgiAdapter::SerializeEntityComponent(
    const GenericEntityId& entity_id, const GenericComponentId& component_id,
    flatbuffers::unique_ptr_t* data_out) {
  if (!EntityExists(entity_id)) return false;
  corgi::EntityRef entity = GetEntityRef(entity_id);
  corgi::ComponentId cid = GetCorgiComponentId(component_id);
  if (!entity || cid == corgi::kInvalidComponent) return false;
  corgi::ComponentInterface* component = entity_manager_->GetComponent(cid);

  auto services = entity_manager_->GetComponent<CommonServicesComponent>();
  bool prev_force_defaults = services->export_force_defaults();
  // Force all default fields to have values that the user can edit
  services->set_export_force_defaults(true);

  flatbuffers::unique_ptr_t raw_data = component->ExportRawData(entity);
  services->set_export_force_defaults(prev_force_defaults);
  if (raw_data != nullptr) {
    if (data_out != nullptr) {
      *data_out = std::move(raw_data);
    }
    return true;
  } else {
    // No output was serialized.
    return false;
  }
}

bool CorgiAdapter::DeserializeEntityComponent(
    const GenericEntityId& entity_id, const GenericComponentId& component_id,
    const uint8_t* data) {
  if (data == nullptr) return false;
  if (!EntityExists(entity_id)) return false;
  corgi::EntityRef entity = GetEntityRef(entity_id);
  corgi::ComponentId cid = GetCorgiComponentId(component_id);
  if (!entity || cid == corgi::kInvalidComponent) return false;
  corgi::ComponentInterface* component = entity_manager_->GetComponent(cid);
  component->AddFromRawData(entity, flatbuffers::GetAnyRoot(data));
  return true;
}

void CorgiAdapter::OverrideFileCache(const std::string& filename,
                                     const std::vector<uint8_t>& data) {
  // TODO: implement
  (void)filename;
  (void)data;
}

bool CorgiAdapter::HighlightEntity(const corgi::EntityRef& entity, float tint) {
  bool did_highlight = false;
  if (!entity) return false;
  auto render_data = entity_manager_->GetComponentData<RenderMeshData>(entity);
  if (render_data != nullptr) {
    render_data->tint = mathfu::vec4(tint, tint, tint, 1);
    did_highlight = true;
  }
  // Highlight the node's children as well.
  auto transform_data =
      entity_manager_->GetComponentData<TransformData>(entity);

  if (transform_data != nullptr) {
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      // Highlight the child, but slightly less brightly.
      if (HighlightEntity(iter->owner, 1 + ((tint - 1) * .8f)))
        did_highlight = true;
    }
  }
  return did_highlight;
}

void CorgiAdapter::CreateDefaultCamera() {
  fplbase::LogInfo("Creating a default BasicCamera for Scene Lab CorgiAdapter");
  camera_.reset(new scene_lab::BasicCamera());
}

corgi::EntityRef CorgiAdapter::GetEntityRef(const GenericEntityId& id) const {
  (void)id;
  if (id == kNoEntityId) return corgi::EntityRef();
  auto meta_component = entity_manager_->GetComponent<MetaComponent>();
  return meta_component->GetEntityFromDictionary(id);
}

GenericEntityId CorgiAdapter::GetEntityId(const corgi::EntityRef& ref) const {
  if (!ref) return kNoEntityId;
  auto meta_component = entity_manager_->GetComponent<MetaComponent>();
  return meta_component->GetEntityID(ref);
}

corgi::ComponentId CorgiAdapter::GetCorgiComponentId(
    const GenericComponentId& id) const {
  if (id == kNoComponentId) return corgi::kInvalidComponent;
  return static_cast<corgi::ComponentId>(atoi(id.c_str()));
}

GenericComponentId CorgiAdapter::GetGenericComponentId(
    corgi::ComponentId c) const {
  if (c == corgi::kInvalidComponent) return kNoComponentId;

  std::stringstream ss;
  ss << c;
  return ss.str();
}

}  // namespace scene_lab_corgi
